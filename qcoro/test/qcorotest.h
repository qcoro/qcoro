// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * This code is very closely based on qtestcase.h header file from Qt
 * Copyright (c) 2016 The Qt Company Ltd., LGPLv3/GPLv2 or GPLv3 or any
 * later version approved by the KDE Free Qt Foundation.
 */

#pragma once

#include <qtestcase.h>
#include <QString>

/**
 * @brief Coroutine-friendly version of QVERIFY test macro.
 **/
#define QCORO_VERIFY(statement)                                                                    \
    do {                                                                                           \
        if (!QTest::qVerify(static_cast<bool>(statement), #statement, "", __FILE__, __LINE__))     \
            co_return;                                                                             \
    } while (false)

/**
 * @brief Coroutine-friendly version of QFAIL test macro.
 **/
#define QCORO_FAIL(message)                                                                        \
    do {                                                                                           \
        QTest::qFail(static_cast<const char *>(message), __FILE__, __LINE__);                      \
        co_return;                                                                                 \
    } while (false)

/**
 * @brief Coroutine-friendly version of QVERIFY2 test macro.
 **/
#define QCORO_VERIFY2(statement, description) \
    do { \
        if (statement) { \
            if (!QTest::qVerify(true, #statement, static_cast<const char *>(description), __FILE__, __LINE__)) \
                co_return; \
        } else { \
            if (!QTest::qVerify(false, #statement, static_cast<const char *>(description), __FILE__, __LINE__)) \
                co_return; \
        } \
    } while (false)

/**
 * @brief Coroutine-friendly version of QCOMPARE test macro.
 **/
#define QCORO_COMPARE(actual, expected)                                                            \
    do {                                                                                           \
        if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))            \
            co_return;                                                                             \
    } while (false)


#if !defined(QT_NO_EXCEPTIONS)

/**
 * @brief Coroutine-friendly version of QVERIFY_THROWS_EXCEPTION macro.
 **/
#define QCORO_VERIFY_THROWS_EXCEPTION(exceptionType, ...)                                          \
    do {                                                                                           \
        try {                                                                                      \
            try {                                                                                  \
                __VA_ARGS__;                                                                       \
                QTest::qFail("Expected exception of type " #exceptionType " to be thrown"          \
                             " but no exception caught",                                           \
                             __FILE__, __LINE__);                                                  \
                co_return;                                                                         \
            } catch (const exceptionType &) {}                                                     \
        } catch (const std::exception &e) {                                                        \
            const QByteArray msg = QByteArray() +                                                  \
                                   "Expected exception of type " #exceptionType                    \
                                   " to be thrown, but std::exception caught with message " +      \
                                   e.what();                                                       \
            QTest::qFail(msg.constData(), __FILE__, __LINE__);                                     \
            co_return;                                                                             \
        } catch (...) {                                                                            \
            QTest::qFail("Expected exception of type " #exceptionType " to be thrown"              \
                         " but unknown exception caught",                                          \
                         __FILE__, __LINE__);                                                      \
            co_return;                                                                             \
        }                                                                                          \
    } while (false)

/**
 * @brief Coroutine-friendly version of QVERIFY_EXCEPTION_THROWN macro.
 **/
#define QCORO_VERIFY_EXCEPTION_THROWN(expression, exceptionType) \
    QCORO_VERIFY_THROWS_EXCEPTION(exceptionType, expression)

#else // QT_NO_EXCEPTIONS

#define QCORO_VERIFY_EXCEPTION_THROWN(expression, exceptionType) \
    static_assert(false, "Support of exceptions is disabled")
#define QCORO_VERIFY_THROWS_EXCEPTION(exceptionType, ...) \
    static_assert(false, "Support for exceptions is disabled")

#endif // QT_NO_EXCEPTIONS

/* @cond INTERNAL */

#define QCORO_TRY_TIMEOUT_DEBUG_IMPL(expr, timeoutValue, step) \
        if (!(expr)) { \
            QTRY_LOOP_IMPL((expr), (2 * timeoutValue), step); \
            if (expr) { \
                QString msg = QString::fromUtf8("QTestLib: This test case check (\"%1\") failed because the requested timeout (%2 ms) was too short, %3 ms would have been sufficient this time."); \
                msg = msg.arg(QString::fromUtf8(#expr)).arg(timeoutValue).arg(timeoutValue + qt_test_i); \
                QCORO_FAIL(qPrintable(msg)); \
            } \
        }

#define QCORO_TRY_IMPL(expr, timeout) \
    const int qcoro_test_step = timeout < 350 ? timeout / 7 + 1 : 50; \
    const int qcoro_test_timeoutValue = timeout; \
    { QCORO_TRY_LOOP_IMPL((expr), qcoro_test_timeoutValue, qcoro_test_step); } \
    QCORO_TRY_TIMEOUT_DEBUG_IMPL((expr), qcoro_test_timeoutValue, qcoro_test_step) \

/* @endcond */

/**
 * @brief Coroutine-friendly version of QVERIFY_WITH_TIMEOUT test macro.
 **/
#define QCORO_TRY_VERIFY_WITH_TIMEOUT(expr, timeout) \
    do { \
        QCORO_TRY_IMPL((expr), timeout); \
        QCORO_VERIFY(expr); \
    } while (false);

/**
 * @brief Coroutine-friendly version of QTRY_VERIFY test macro.
 **/
#define QCORO_TRY_VERIFY(expr) QCORO_TRY_VERIFY_WITH_TIMEOUT((expr), 5000)

/**
 * @brief Coroutine-friendly version of QTRY_VERIFY2_WITH_TIMEOUT test macro.
 **/
#define QCORO_TRY_VERIFY2_WITH_TIMEOUT(expr, messageExpression, timeout) \
    do { \
        QCORO_TRY_IMPL((expr), timeout); \
        QCORO_VERIFY2((expr), messageExpression); \
    } while (false)

/**
 * @brief Coroutine-friendly version of QTRY_VERIFY2 test macro.
 **/
#define QCORO_TRY_VERIFY2(expr, messageExpression) \
    QCORO_TRY_VERIFY2_WITH_TIMEOUT((expr), (messageExpression), 5000)

/**
 * @brief Coroutine-friendly version of QTRY_COMPARE_WITH_TIMEOUT test macro.
 **/
#define QCORO_TRY_COMPARE_WITH_TIMEOUT(expr, expected, timeout) \
    do { \
        QCORO_TRY_IMPL(((expr) == (expected)), timeout); \
        QCORO_COMPARE((expr), expected); \
    } while (false)

/**
 * @brief Coroutine-friendly version of QTRY_COMPARE test macro.
 **/
#define QCORO_TRY_COMPARE(expr, expected) \
    QCORO_TRY_COMPARE_WITH_TIMEOUT((expr), (expected), 5000)

/**
 * @internal
 **/
#define QCORO_SKIP_INTERNAL(statement) \
    do { \
        QTest::qSkip(static_cast<const char *>(statement), __FILE__, __LINE__); \
        co_return; \
    } while (false)

/**
 * @brief Coroutine-friendly version of QSKIP test macro.
 **/
#define QCORO_SKIP(statement, ...) QCORO_SKIP_INTERNAL(statement)

/**
 * @brief Coroutine-friendly version of QEXPECT_FAIL test macro.
 **/
#define QCORO_EXPECT_FAIL(dataIndex, comment, mode) \
    do { \
        if (!QTest::qExpectFail(dataIndex, static_cast<const char *>(comment), QTest::mode, __FILE__, __LINE__)) \
        co_return; \
    } while (false)

/**
 * @brief Coroutine-friendly version of QTEST test macro.
 **/
#define QCORO_TEST(actual, testElement) \
    do { \
        if (!QTest::qTest(actual, testElement, #actual, #testElement, __FILE__, __LINE__)) \
            co_return; \
    } while (false)


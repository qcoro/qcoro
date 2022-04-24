// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once


//! Executes given \c expr with 10ms delay.
#define QCORO_DELAY(expr)                                                                          \
    QTimer::singleShot(10ms, [&]() { expr; })

#define QCORO_VERIFY(statement)                                                                    \
    do {                                                                                           \
        if (!QTest::qVerify(static_cast<bool>(statement), #statement, "", __FILE__, __LINE__))     \
            co_return;                                                                             \
    } while (false)

#define QCORO_COMPARE(actual, expected)                                                            \
    do {                                                                                           \
        if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))            \
            co_return;                                                                             \
    } while (false)

#define QCORO_FAIL(message)                                                                        \
    do {                                                                                           \
        QTest::qFail(static_cast<const char *>(message), __FILE__, __LINE__);                      \
        co_return;                                                                                 \
    } while (false)

#define QCORO_VERIFY_EXCEPTION_THROWN(expression, exceptionType)                                   \
    do {                                                                                           \
        try {                                                                                      \
            try {                                                                                  \
                expression;                                                                        \
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


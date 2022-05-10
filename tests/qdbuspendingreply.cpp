// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorodbustestinterface.h"
#include "testdbusserver.h"
#include "testobject.h"

#include "qcorodbuspendingreply.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>

#include <memory>
#include <thread>

class QCoroDBusPendingCallTest : public QCoro::TestObject<QCoroDBusPendingCallTest> {
    Q_OBJECT
private:
    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const auto resp = co_await iface.foo();

        QCORO_VERIFY(resp.isFinished());
    }

    QCoro::Task<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                        QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const auto resp = co_await qCoro(iface.foo()).waitForFinished();

        QCORO_VERIFY(resp.isFinished());
    }

    void testThenQCoroWrapperTriggers_coro(TestLoop &el) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QVERIFY(iface.isValid());

        bool called = false;
        qCoro(iface.foo()).waitForFinished().then([&](QDBusPendingReply<> reply) {
            called = true;
            el.quit();
            QVERIFY(reply.isFinished());
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testReturnsResult_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const QString reply = co_await iface.ping(QStringLiteral("Hello there!"));

        QCORO_COMPARE(reply, QStringLiteral("Hello there!"));
    }

    void testThenReturnsResult_coro(TestLoop &el) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QVERIFY(iface.isValid());

        bool called = false;
        qCoro(iface.ping(QStringLiteral("Hello there!"))).waitForFinished().then(
            [&](const QDBusPendingReply<QString> &reply) {
                called = true;
                el.quit();
                QCOMPARE(reply.value(), QStringLiteral("Hello there!"));
            });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testReturnsBlockingResult_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const QString reply = co_await iface.blockAndReturn(1);

        QCORO_COMPARE(reply, QStringLiteral("Slept for 1 seconds"));
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const auto result = co_await iface.blockFor(1);

        QCORO_VERIFY(result.isFinished());
        QCORO_VERIFY(eventLoopResponsive);
    }

    QCoro::Task<> testDoesntCoAwaitFinishedCall_coro(QCoro::TestContext test) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        auto call = iface.foo();
        QDBusReply<void> reply = co_await call;
        QCORO_VERIFY(reply.isValid());

        test.setShouldNotSuspend();

        reply = co_await call;

        QCORO_VERIFY(reply.isValid());
    }

    void testThenDoesntCoAwaitFinishedCall_coro(TestLoop &el) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QVERIFY(iface.isValid());

        auto call = iface.foo();
        call.waitForFinished();

        bool called = false;
        qCoro(call).waitForFinished().then([&](QDBusPendingReply<>) {
            called = true;
            el.quit();
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testHandlesMultipleArguments_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        QDBusPendingReply<QString, bool> reply = iface.asyncCall(QStringLiteral("blockAndReturnMultipleArguments"), 1);
        co_await reply;

        QCORO_VERIFY(reply.isFinished());
        QCORO_COMPARE(reply.argumentAt<0>(), QStringLiteral("Hello World!"));
        QCORO_COMPARE(reply.argumentAt<1>(), true);
    }

    void testThenHandlesMultipleArguments_coro(TestLoop &el) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath,
                                         QDBusConnection::sessionBus());
        QVERIFY(iface.isValid());

        QDBusPendingReply<QString, bool> reply = iface.asyncCall(QStringLiteral("blockAndReturnMultipleArguments"), 1);
        bool called = false;
        qCoro(reply).waitForFinished().then([&](const QDBusPendingReply<QString, bool> &reply) {
            called = true;
            el.quit();
            QCOMPARE(reply.argumentAt<0>(), QStringLiteral("Hello World!"));
            QCOMPARE(reply.argumentAt<1>(), true);
        });
        el.exec();
        QVERIFY(called);
    }

private Q_SLOTS:
    void initTestCase() {
        for (int i = 0; i < 10; ++i) {
            QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath,
                                 DBusServer::interfaceName);
            if (iface.isValid()) {
                return;
            }
            QTest::qWait(100);
        }

        QFAIL("Failed to obtain a valid dbus interface");
    }

    void cleanupTestCase() {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath,
                             DBusServer::interfaceName);
        iface.call(QStringLiteral("quit"));
    }

    addTest(Triggers)
    addCoroAndThenTests(QCoroWrapperTriggers)
    addCoroAndThenTests(ReturnsResult)
    addTest(ReturnsBlockingResult)
    addTest(DoesntBlockEventLoop)
    addCoroAndThenTests(DoesntCoAwaitFinishedCall)
    addCoroAndThenTests(HandlesMultipleArguments)
};

DBUS_TEST_MAIN(QCoroDBusPendingCallTest)

#include "qdbuspendingreply.moc"

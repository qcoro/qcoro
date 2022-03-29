// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testdbusserver.h"
#include "testobject.h"

#include "qcorodbuspendingcall.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>

class QCoroDBusPendingCallTest : public QCoro::TestObject<QCoroDBusPendingCallTest> {
    Q_OBJECT
private:
    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath,
                             DBusServer::interfaceName);
        QCORO_VERIFY(iface.isValid());

        const QDBusReply<void> reply = co_await iface.asyncCall(QStringLiteral("foo"));
        QCORO_VERIFY(reply.isValid());
    }

    QCoro::Task<> testReturnsResult_coro(QCoro::TestContext) {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath,
                             DBusServer::interfaceName);
        QCORO_VERIFY(iface.isValid());

        const QDBusReply<QString> reply =
            co_await iface.asyncCall(QStringLiteral("ping"), QStringLiteral("Hello there!"));

        QCORO_VERIFY(reply.isValid());
        QCORO_COMPARE(reply.value(), QStringLiteral("Hello there!"));
    }

    void testThenReturnsResult_coro(TestLoop &el) {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath,
                             DBusServer::interfaceName);
        QVERIFY(iface.isValid());

        const QDBusPendingCall call =
            iface.asyncCall(QStringLiteral("ping"), QStringLiteral("Hello there!"));
        bool called = false;
        qCoro(call).waitForFinished().then([&](const QDBusMessage &msg) {
            called = true;
            el.quit();
            QCOMPARE(QDBusReply<QString>(msg).value(), QStringLiteral("Hello there!"));
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath,
                             DBusServer::interfaceName);
        QCORO_VERIFY(iface.isValid());

        const QDBusReply<void> reply = co_await iface.asyncCall(QStringLiteral("blockFor"), 1);

        QCORO_VERIFY(reply.isValid());
        QCORO_VERIFY(eventLoopResponsive);
    }

    QCoro::Task<> testDoesntCoAwaitFinishedCall_coro(QCoro::TestContext test) {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath,
                             DBusServer::interfaceName);
        QCORO_VERIFY(iface.isValid());

        auto call = iface.asyncCall(QStringLiteral("foo"));
        QDBusReply<void> reply = co_await call;
        QCORO_VERIFY(reply.isValid());

        test.setShouldNotSuspend();

        reply = co_await call;

        QCORO_VERIFY(reply.isValid());
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
    addCoroAndThenTests(ReturnsResult)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitFinishedCall)
};

DBUS_TEST_MAIN(QCoroDBusPendingCallTest)

#include "qdbuspendingcall.moc"

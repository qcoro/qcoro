// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/dbus.h"
#include "testdbusserver.h"
#include "qcorodbustestinterface.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusError>

#include <thread>
#include <memory>

class QCoroDBusPendingCallTest: public QCoro::TestObject<QCoroDBusPendingCallTest> {
    Q_OBJECT
private:

    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath, QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const auto resp = co_await iface.foo();

        QCORO_VERIFY(resp.isFinished());
    }

    QCoro::Task<> testReturnsResult_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath, QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const QString reply = co_await iface.ping("Hello there!");

        QCORO_COMPARE(reply, QStringLiteral("Hello there!"));
    }

    QCoro::Task<> testReturnsBlockingResult_coro(QCoro::TestContext) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath, QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const QString reply = co_await iface.blockAndReturn(1);

        QCORO_COMPARE(reply, QStringLiteral("Slept for 1 seconds"));
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath, QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        const auto result = co_await iface.blockFor(1);

        QCORO_VERIFY(result.isFinished());
        QCORO_VERIFY(eventLoopResponsive);
    }

    QCoro::Task<> testDoesntCoAwaitFinishedCall_coro(QCoro::TestContext test) {
        cz::dvratil::qcorodbustest iface(DBusServer::serviceName, DBusServer::objectPath, QDBusConnection::sessionBus());
        QCORO_VERIFY(iface.isValid());

        auto call = iface.foo();
        QDBusReply<void> reply = co_await call;
        QCORO_VERIFY(reply.isValid());

        test.setShouldNotSuspend();

        reply = co_await call;

        QCORO_VERIFY(reply.isValid());
    }

private Q_SLOTS:
    void initTestCase() {
        for (int i = 0; i < 10; ++i) {
            QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath, DBusServer::interfaceName);
            if (iface.isValid()) {
                return;
            }
            QTest::qWait(100);
        }

        QFAIL("Failed to obtain a valid dbus interface");
    }

    void cleanupTestCase() {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath, DBusServer::interfaceName);
        iface.call(QStringLiteral("quit"));
    }

    addTest(Triggers)
    addTest(ReturnsResult)
    addTest(ReturnsBlockingResult)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitFinishedCall)
};


DBUS_TEST_MAIN(QCoroDBusPendingCallTest)

#include "qdbuspendingreply.moc"


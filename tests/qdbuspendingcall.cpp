// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/dbus.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusError>

#include <thread>
#include <memory>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class DBusServer : public QObject
{
    Q_OBJECT

public:

    inline static const QString serviceName = QStringLiteral("cz.dvratil.qcorodbustest");
    inline static const QString interfaceName = QStringLiteral("cz.dvratil.qcorodbustest");
    inline static const QString objectPath = QStringLiteral("/");

    DBusServer() {
        QTimer::singleShot(0, this, &DBusServer::run);
    }

    void run() {
        auto conn = QDBusConnection::sessionBus();
        if (!conn.registerService(serviceName)) {
            qWarning() << "Failed to register service to DBus:" << conn.lastError().message();
        }
        if (!conn.registerObject(objectPath, interfaceName, this, QDBusConnection::ExportAllSlots)) {
            qWarning() << "Failed to register object to DBus" << conn.lastError().message();
        }
    }

public Q_SLOTS:
    void foo() {};

    void blockFor(int seconds) {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }

    QString ping(const QString &ping) {
        return ping;
    }

    void quit() {
        qApp->quit();
    }
};


class QCoroDBusPendingCallTest: public QCoro::TestObject<QCoroDBusPendingCallTest> {
    Q_OBJECT
private:

    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath, DBusServer::interfaceName);
        QCORO_VERIFY(iface.isValid());

        const QDBusReply<void> reply = co_await iface.asyncCall(QStringLiteral("foo"));
        QCORO_VERIFY(reply.isValid());
    }

    QCoro::Task<> testReturnsResult_coro(QCoro::TestContext) {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath, DBusServer::interfaceName);
        QCORO_VERIFY(iface.isValid());

        const QDBusReply<QString> reply = co_await iface.asyncCall(QStringLiteral("ping"), QStringLiteral("Hello there!"));

        QCORO_VERIFY(reply.isValid());
        QCORO_COMPARE(reply.value(), QStringLiteral("Hello there!"));
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath, DBusServer::interfaceName);
        QCORO_VERIFY(iface.isValid());

        const QDBusReply<void> reply = co_await iface.asyncCall(QStringLiteral("blockFor"), 1);

        QCORO_VERIFY(reply.isValid());
        QCORO_VERIFY(eventLoopResponsive);
    }

    QCoro::Task<> testDoesntCoAwaitFinishedCall_coro(QCoro::TestContext test) {
        QDBusInterface iface(DBusServer::serviceName, DBusServer::objectPath, DBusServer::interfaceName);
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
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitFinishedCall)
};

int runDBusServer(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    DBusServer server;
    return app.exec();
}

int runTest(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCoroDBusPendingCallTest tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

// We must fork the DBus server into its own process due to QTBUG-92107 (asyncCall blocks if the
// remote service is registered in the same process (even if it lives in a different thread)
int main(int argc, char **argv) {
    const auto child = fork();
    if (child == 0) {
        return runDBusServer(argc, argv);
    }

    const int result = runTest(argc, argv);
    waitpid(child, 0, WNOHANG);
    return result;
}

#include "qdbuspendingcall.moc"


// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include <QCoreApplication>
#include <QObject>
#include <QProcess>
#include <QTimer>

#include <QString>

#include <iostream>

class DBusServer : public QObject {
    Q_OBJECT

public:
    inline static const QString serviceName = QStringLiteral("cz.dvratil.qcorodbustest");
    inline static const QString interfaceName = QStringLiteral("cz.dvratil.qcorodbustest");
    inline static const QString objectPath = QStringLiteral("/");

    explicit DBusServer();

    void run();

public Q_SLOTS:

    void foo();
    QString ping(const QString &ping);

    void blockFor(int seconds);
    QString blockAndReturn(int seconds);
    QString blockAndReturnMultipleArguments(int seconds, bool &out);

    void quit();

private:
    QTimer mSuicideTimer;
};

// We must run the DBus server into its own process due to QTBUG-92107 (asyncCall blocks if the
// remote service is registered in the same process (even if it lives in a different thread)
#define DBUS_TEST_MAIN(TestClass)                                                                  \
    bool startDBusServer(QProcess &process) {                                                      \
        process.start(QStringLiteral(TESTDBUSSERVER_EXECUTABLE), QStringList{});                   \
        if (!process.waitForStarted()) {                                                           \
            std::cerr << "Failed to start testdbusserver" << std::endl;                            \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    bool stopDBusServer(QProcess &process) {                                                       \
        process.waitForFinished();                                                                 \
        if (process.exitCode() != 0) {                                                             \
            std::cerr << "testdbuserver terminated with exit code " << process.exitCode()          \
                      << std::endl;                                                                \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    int main(int argc, char **argv) {                                                              \
        QCoreApplication app(argc, argv);                                                          \
        QProcess dbusServer;                                                                       \
        if (!startDBusServer(dbusServer))                                                          \
            return 1;                                                                              \
        TestClass testClass;                                                                       \
        QTEST_SET_MAIN_SOURCE_PATH                                                                 \
        const int result = QTest::qExec(&testClass, argc, argv);                                   \
        if (!stopDBusServer(dbusServer))                                                           \
            return 1;                                                                              \
        return result;                                                                             \
    }

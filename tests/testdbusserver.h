// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include <QObject>

#include <QString>

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

    explicit DBusServer();

    void run();

public Q_SLOTS:

    void foo();
    QString ping(const QString &ping);

    void blockFor(int seconds);
    QString blockAndReturn(int seconds);

    void quit();
};

// We must fork the DBus server into its own process due to QTBUG-92107 (asyncCall blocks if the
// remote service is registered in the same process (even if it lives in a different thread)
#define DBUS_TEST_MAIN(TestClass) \
    int main(int argc, char **argv) { \
        const auto child = fork(); \
        if (child == 0) { \
            QCoreApplication app(argc, argv); \
            DBusServer server; \
            return app.exec(); \
        } \
        \
        QCoreApplication app(argc, argv); \
        TestClass testClass; \
        QTEST_SET_MAIN_SOURCE_PATH \
        const int result = QTest::qExec(&testClass, argc, argv); \
        waitpid(child, 0, WNOHANG); \
        return result;  \
    }



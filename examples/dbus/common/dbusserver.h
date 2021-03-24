#pragma once

#include <QObject>

class DBusServer: public QObject
{
    Q_OBJECT
public:
    static const QString serviceName;
    static const QString objectPath;
    static const QString interfaceName;

    explicit DBusServer(QObject *parent = nullptr);

    //! Starts an event loop and runs the DBus server.
    /*! Blocks until the main QApplication quits. */
    void run();

public Q_SLOTS:
    QString blockingPing(int seconds) const;
};


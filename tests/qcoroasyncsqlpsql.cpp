#include "qcoroasyncsqlbase.h"

#include <QTemporaryDir>
#include <QProcess>

namespace
{

QByteArray runProgram(const QString &program, const QStringList &args)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(args);
    process.start();
    process.waitForFinished();
    if (process.exitCode() != 0) {
        return process.readAllStandardError() + process.readAllStandardOutput();
    }
    return {};
}

}

class QCoroAsyncSqlPsqlTest final : public QCoroAsyncSqlTestBase
{
    Q_OBJECT
public:
    explicit QCoroAsyncSqlPsqlTest(): QCoroAsyncSqlTestBase(QStringLiteral("QPSQL"))
    {}

protected:
    bool startDatabase() override
    {
        QDir().mkpath(mDataDir.path());
        qDebug() << "Initializing db in" << mDataDir.path();
        if (const auto err = runProgram(QStringLiteral("initdb"), {QStringLiteral("-D"), mDataDir.path()}); !err.isEmpty()) {
            qWarning() << "Failed to initialize database:" << err;
            return false;
        }

        qDebug() << "Starting database";
        if (const auto err = runProgram(QStringLiteral("pg_ctl"), { QStringLiteral("start"), QStringLiteral("-D"), mDataDir.path(), QStringLiteral("-o"), QStringLiteral("-k %1 -h ''").arg(mDataDir.path()) }); !err.isEmpty()) {
            qWarning() << "Failed to start database:" << err;
            return false;
        }

        qDebug() << "Creating database";
        if (const auto err = runProgram(QStringLiteral("createdb"), {QStringLiteral("-h"), mDataDir.path(), QStringLiteral("test_db")}); !err.isEmpty()) {
            qWarning() << "Failed to create database:" << err;
            return false;
        }

        return true;
    }

    bool stopDatabase() override
    {
        qDebug() << "Stopping database";
        if (const auto err = runProgram(QStringLiteral("pg_ctl"), { QStringLiteral("stop"), QStringLiteral("-D"), mDataDir.path(), QStringLiteral("-m"), QStringLiteral("fast")}); !err.isEmpty()) {
            qWarning() << "Failed to stop database:" << err;
            return false;
        }
        return true;
    }

    bool configureDatabase(QCoro::AsyncSqlDatabase &db) override
    {
        db.setHostName(mDataDir.path());
        db.setDatabaseName(QStringLiteral("test_db"));
        return true;
    }

private:
    QTemporaryDir mDataDir;
};

QTEST_GUILESS_MAIN(QCoroAsyncSqlPsqlTest)

#include "qcoroasyncsqlpsql.moc"
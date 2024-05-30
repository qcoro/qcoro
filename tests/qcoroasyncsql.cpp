#include <QTest>

#include "qcoroasyncsqldatabase.h"

class QCoroAsyncSqlTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QCoreApplication::addLibraryPath(const QString &)
    }

    void testDrivers_data()
    {
        QTest::addColumn<QString>("driver");

        QTest::newRow("QMYSQL") << QStringLiteral("QMYSQL");
        QTest::newRow("QSQLITE") << QStringLiteral("QSQLITE");
        QTest::newRow("QPSQL") << QStringLiteral("QPSQL");
    }

    void testDrivers()
    {
        QFETCH(QString, driver);

        QVERIFY(QCoro::AsyncSqlDatabase::drivers().contains(driver));
    }
};

QTEST_GUILESS_MAIN(QCoroAsyncSqlTest)

#include "qcoroasyncsql.moc"
#include "qcoroasyncsqlbase.h"

#include <QSqlError>

QCoroAsyncSqlTestBase::QCoroAsyncSqlTestBase(const QString &driver)
    : mDriverName(driver)
{}

void QCoroAsyncSqlTestBase::initTestCase()
{
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() % u"/plugins");

    QVERIFY(startDatabase());
}

void QCoroAsyncSqlTestBase::cleanupTestCase()
{
    QVERIFY(stopDatabase());
}

void QCoroAsyncSqlTestBase::testDrivers()
{
    QVERIFY(QCoro::AsyncSqlDatabase::drivers().contains(mDriverName));
}

QCoro::Task<> QCoroAsyncSqlTestBase::testOpen_coro(QCoro::TestContext)
{
    auto db = QCoro::AsyncSqlDatabase::addDatabase(mDriverName);
    QCORO_VERIFY(configureDatabase(db));

    if (!co_await db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        QCORO_FAIL("Failed to open DB");
    }
    QCORO_VERIFY(db.isOpen());
}
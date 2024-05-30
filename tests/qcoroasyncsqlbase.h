#include <QTest>

#include "config.h"
#include "qcoroasyncsqldatabase.h"
#include "testobject.h"

using namespace Qt::StringLiterals;

class QCoroAsyncSqlTestBase : public QCoro::TestObject<QCoroAsyncSqlTestBase>
{
    Q_OBJECT
protected:
    QString mDriverName;

protected:
    explicit QCoroAsyncSqlTestBase(const QString &driver);
    ~QCoroAsyncSqlTestBase() override = default;

    virtual bool startDatabase() = 0;
    virtual bool stopDatabase() = 0;
    virtual bool configureDatabase(QCoro::AsyncSqlDatabase &db) = 0;

private Q_SLOTS:
    virtual void initTestCase();
    virtual void cleanupTestCase();

    void testDrivers();

    addTest(Open)

private:
    QCoro::Task<> testOpen_coro(QCoro::TestContext);
};
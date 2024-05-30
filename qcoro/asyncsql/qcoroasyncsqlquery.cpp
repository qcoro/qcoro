#include "qcoroasyncsqlquery.h"
#include "qcoroasyncsqlresult.h"
#include "qcoroasyncsqlnulldriver_p.h"
#include "qcoroasyncsqlnullresult_p.h"
#include "qcoroasyncsql_log.h"

#include <QSqlRecord>

using namespace QCoro;

namespace QCoro
{

class AsyncSqlQueryPrivate
{
public:
    AsyncSqlQueryPrivate(AsyncSqlResult* result);
    ~AsyncSqlQueryPrivate();
    QAtomicInt ref;
    AsyncSqlResult* sqlResult;

    static AsyncSqlQueryPrivate* shared_null();
};

} // namespace

Q_GLOBAL_STATIC_WITH_ARGS(AsyncSqlQueryPrivate, nullQueryPrivate, (nullptr))
Q_GLOBAL_STATIC(AsyncSqlNullDriver, nullDriver)
Q_GLOBAL_STATIC_WITH_ARGS(AsyncSqlNullResult, nullResult, (nullDriver()))

AsyncSqlQueryPrivate* AsyncSqlQueryPrivate::shared_null()
{
    AsyncSqlQueryPrivate *null = nullQueryPrivate();
    null->ref.ref();
    return null;
}

AsyncSqlQueryPrivate::AsyncSqlQueryPrivate(AsyncSqlResult* result)
    : ref(1), sqlResult(result)
{
    if (!sqlResult) {
        sqlResult = nullResult();
    }
}

AsyncSqlQueryPrivate::~AsyncSqlQueryPrivate()
{
    AsyncSqlResult *nr = nullResult();
    if (!nr || sqlResult == nr) {
        return;
    }
    delete sqlResult;
}

AsyncSqlQuery::AsyncSqlQuery(AsyncSqlResult *result)
    : d(new AsyncSqlQueryPrivate(result))
{}

AsyncSqlQuery::AsyncSqlQuery(AsyncSqlQuery &&other) noexcept
    : d(std::exchange(other.d, nullptr))
{
}

AsyncSqlQuery &AsyncSqlQuery::operator=(AsyncSqlQuery &&other) noexcept
{
    AsyncSqlQuery moved(std::move(other));
    swap(moved);
    return *this;
}

AsyncSqlQuery::~AsyncSqlQuery()
{
    if (d && !d->ref.deref()) {
        delete d;
    }
}

static Task<void> qInit(AsyncSqlQuery *q, const QString& query, const AsyncSqlDatabase &db)
{
    AsyncSqlDatabase database = db;
    if (!database.isValid()) {
        database = co_await AsyncSqlDatabase::database(QLatin1StringView(AsyncSqlDatabase::defaultConnection), false);
    }
    if (database.isValid()) {
        *q = AsyncSqlQuery(database.driver()->createResult());
    }

    if (!query.isEmpty()) {
        co_await q->exec(query);
    }
}

AsyncSqlQuery::AsyncSqlQuery(const QString& query, const AsyncSqlDatabase &db)
    : d(AsyncSqlQueryPrivate::shared_null())
{
    qInit(this, query, db);
}

AsyncSqlQuery::AsyncSqlQuery(const AsyncSqlDatabase &db)
    : AsyncSqlQuery(QString(), db)
{
}

bool AsyncSqlQuery::isNull(int field) const
{
    return !d->sqlResult->isActive()
             || !d->sqlResult->isValid()
             || d->sqlResult->isNull(field);
}

Task<bool> AsyncSqlQuery::isNull(const QString &name) const
{
    const qsizetype index = (co_await d->sqlResult->record()).indexOf(name);
    if (index >= -1) {
        co_return isNull(index);
    }
    qCWarning(AsyncSqlLog, "AsyncSqlQuery::isNull: unknown field name '%ls'", qUtf16Printable(name));
    co_return true;
}

Task<bool> AsyncSqlQuery::exec(const QString& query)
{
    if (!driver()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::exec: called before driver has been set up");
        co_return false;
    }

    if (d->ref.loadRelaxed() != 1) {
        bool fo = isForwardOnly();
        *this = AsyncSqlQuery(driver()->createResult());
        d->sqlResult->setNumericalPrecisionPolicy(d->sqlResult->numericalPrecisionPolicy());
        setForwardOnly(fo);
    } else {
        d->sqlResult->clear();
        d->sqlResult->setActive(false);
        d->sqlResult->setLastError(QSqlError());
        d->sqlResult->setAt(QSql::BeforeFirstRow);
        d->sqlResult->setNumericalPrecisionPolicy(d->sqlResult->numericalPrecisionPolicy());
    }
    d->sqlResult->setQuery(query.trimmed());
    if (!driver()->isOpen() || driver()->isOpenError()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::exec: database not open");
        co_return false;
    }
    if (query.isEmpty()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::exec: empty query");
        co_return false;
    }

    co_return co_await d->sqlResult->reset(query);
}

QVariant AsyncSqlQuery::value(int index) const
{
    if (isActive() && isValid() && (index > -1)) {
        return d->sqlResult->data(index);
    }
    qCWarning(AsyncSqlLog, "AsyncSqlQuery::value: not positioned on a valid record");
    return QVariant();
}

Task<QVariant> AsyncSqlQuery::value(const QString &name) const
{
    const qsizetype index = (co_await d->sqlResult->record()).indexOf(name);
    if (index > -1) {
        co_return value(index);
    }
    qCWarning(AsyncSqlLog, "AsyncSqlQuery::value: unknown field name '%ls'", qUtf16Printable(name));
    co_return QVariant();
}

int AsyncSqlQuery::at() const
{
    return d->sqlResult->at();
}

QString AsyncSqlQuery::lastQuery() const
{
    return d->sqlResult->lastQuery();
}

const AsyncSqlDriver *AsyncSqlQuery::driver() const
{
    return d->sqlResult->driver();
}

const AsyncSqlResult* AsyncSqlQuery::result() const
{
    return d->sqlResult;
}

Task<bool> AsyncSqlQuery::seek(int index, bool relative)
{
    if (!isSelect() || !isActive()) {
        co_return false;
    }

    int actualIdx;
    if (!relative) { // arbitrary seek
        if (index < 0) {
            d->sqlResult->setAt(QSql::BeforeFirstRow);
            co_return false;
        }
        actualIdx = index;
    } else {
        switch (at()) { // relative seek
        case QSql::BeforeFirstRow:
            if (index > 0) {
                actualIdx = index - 1;
            } else {
                co_return false;
            }
            break;
        case QSql::AfterLastRow:
            if (index < 0) {
                d->sqlResult->fetchLast();
                actualIdx = at() + index + 1;
            } else {
                co_return false;
            }
            break;
        default:
            if ((at() + index) < 0) {
                d->sqlResult->setAt(QSql::BeforeFirstRow);
                co_return false;
            }
            actualIdx = at() + index;
            break;
        }
    }
    // let drivers optimize
    if (isForwardOnly() && actualIdx < at()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::seek: cannot seek backwards in a forward only query");
        co_return false;
    }
    if (actualIdx == (at() + 1) && at() != QSql::BeforeFirstRow) {
        if (!co_await d->sqlResult->fetchNext()) {
            d->sqlResult->setAt(QSql::AfterLastRow);
            co_return false;
        }
        co_return true;
    }
    if (actualIdx == (at() - 1)) {
        if (!co_await d->sqlResult->fetchPrevious()) {
            d->sqlResult->setAt(QSql::BeforeFirstRow);
            co_return false;
        }
        co_return true;
    }
    if (!co_await d->sqlResult->fetch(actualIdx)) {
        d->sqlResult->setAt(QSql::AfterLastRow);
        co_return false;
    }
    co_return true;
}

Task<bool> AsyncSqlQuery::next()
{
    if (!isSelect() || !isActive()) {
        co_return false;
    }

    switch (at()) {
    case QSql::BeforeFirstRow:
        co_return co_await d->sqlResult->fetchFirst();
    case QSql::AfterLastRow:
        co_return false;
    default:
        if (!co_await d->sqlResult->fetchNext()) {
            d->sqlResult->setAt(QSql::AfterLastRow);
            co_return false;
        }
        co_return true;
    }
}

Task<bool> AsyncSqlQuery::previous()
{
    if (!isSelect() || !isActive()) {
        co_return false;
    }
    if (isForwardOnly()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::seek: cannot seek backwards in a forward only query");
        co_return false;
    }

    switch (at()) {
    case QSql::BeforeFirstRow:
        co_return false;
    case QSql::AfterLastRow:
        co_return co_await d->sqlResult->fetchLast();
    default:
        if (!co_await d->sqlResult->fetchPrevious()) {
            d->sqlResult->setAt(QSql::BeforeFirstRow);
            co_return false;
        }
        co_return true;
    }
}

Task<bool> AsyncSqlQuery::first()
{
    if (!isSelect() || !isActive()) {
        co_return false;
    }
    if (isForwardOnly() && at() > QSql::BeforeFirstRow) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::seek: cannot seek backwards in a forward only query");
        co_return false;
    }
    co_return co_await d->sqlResult->fetchFirst();
}

Task<bool> AsyncSqlQuery::last()
{
    if (!isSelect() || !isActive()) {
        co_return false;
    }

    co_return co_await d->sqlResult->fetchLast();
}

int AsyncSqlQuery::size() const
{
    if (isActive() && d->sqlResult->driver()->hasFeature(QSqlDriver::QuerySize)) {
        return d->sqlResult->size();
    }
    return -1;
}

int AsyncSqlQuery::numRowsAffected() const
{
    if (isActive()) {
        return d->sqlResult->numRowsAffected();
    }
    return -1;
}

QSqlError AsyncSqlQuery::lastError() const
{
    return d->sqlResult->lastError();
}

bool AsyncSqlQuery::isValid() const
{
    return d->sqlResult->isValid();
}

bool AsyncSqlQuery::isActive() const
{
    return d->sqlResult->isActive();
}

bool AsyncSqlQuery::isSelect() const
{
    return d->sqlResult->isSelect();
}

bool AsyncSqlQuery::isForwardOnly() const
{
    return d->sqlResult->isForwardOnly();
}

void AsyncSqlQuery::setForwardOnly(bool forward)
{
    d->sqlResult->setForwardOnly(forward);
}

Task<QSqlRecord> AsyncSqlQuery::record() const
{
    QSqlRecord rec = co_await d->sqlResult->record();

    if (isValid()) {
        for (int i = 0; i < rec.count(); ++i) {
            rec.setValue(i, value(i));
        }
    }
    co_return rec;
}

void AsyncSqlQuery::clear()
{
    *this = AsyncSqlQuery(driver()->createResult());
}

Task<bool> AsyncSqlQuery::prepare(const QString& query)
{
    if (d->ref.loadRelaxed() != 1) {
        bool fo = isForwardOnly();
        *this = AsyncSqlQuery(driver()->createResult());
        setForwardOnly(fo);
        d->sqlResult->setNumericalPrecisionPolicy(d->sqlResult->numericalPrecisionPolicy());
    } else {
        d->sqlResult->setActive(false);
        d->sqlResult->setLastError(QSqlError());
        d->sqlResult->setAt(QSql::BeforeFirstRow);
        d->sqlResult->setNumericalPrecisionPolicy(d->sqlResult->numericalPrecisionPolicy());
    }
    if (!driver()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::prepare: no driver");
        co_return false;
    }
    if (!driver()->isOpen() || driver()->isOpenError()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::prepare: database not open");
        co_return false;
    }
    if (query.isEmpty()) {
        qCWarning(AsyncSqlLog, "AsyncSqlQuery::prepare: empty query");
        co_return false;
    }
    co_return co_await d->sqlResult->safePrepare(query);
}

Task<bool> AsyncSqlQuery::exec()
{
    d->sqlResult->resetBindCount();

    if (d->sqlResult->lastError().isValid()) {
        d->sqlResult->setLastError(QSqlError());
    }

    co_return co_await d->sqlResult->exec();
}

Task<bool> AsyncSqlQuery::execBatch(BatchExecutionMode mode)
{
    d->sqlResult->resetBindCount();
    return d->sqlResult->execBatch(mode == ValuesAsColumns);
}

void AsyncSqlQuery::bindValue(const QString& placeholder, const QVariant& val,
                              QSql::ParamType paramType)
{
    d->sqlResult->bindValue(placeholder, val, paramType);
}

void AsyncSqlQuery::bindValue(int pos, const QVariant& val, QSql::ParamType paramType)
{
    d->sqlResult->bindValue(pos, val, paramType);
}

void AsyncSqlQuery::addBindValue(const QVariant& val, QSql::ParamType paramType)
{
    d->sqlResult->addBindValue(val, paramType);
}

QVariant AsyncSqlQuery::boundValue(const QString& placeholder) const
{
    return d->sqlResult->boundValue(placeholder);
}

QVariant AsyncSqlQuery::boundValue(int pos) const
{
    return d->sqlResult->boundValue(pos);
}

QVariantList AsyncSqlQuery::boundValues() const
{
    return QVariantList(d->sqlResult->boundValues());
}

QStringList AsyncSqlQuery::boundValueNames() const
{
    return d->sqlResult->boundValueNames();
}

QString AsyncSqlQuery::boundValueName(int pos) const
{
    return d->sqlResult->boundValueName(pos);
}

QString AsyncSqlQuery::executedQuery() const
{
    return d->sqlResult->executedQuery();
}

Task<QVariant> AsyncSqlQuery::lastInsertId() const
{
    return d->sqlResult->lastInsertId();
}

void AsyncSqlQuery::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
    d->sqlResult->setNumericalPrecisionPolicy(precisionPolicy);
}

QSql::NumericalPrecisionPolicy AsyncSqlQuery::numericalPrecisionPolicy() const
{
    return d->sqlResult->numericalPrecisionPolicy();
}

void AsyncSqlQuery::setPositionalBindingEnabled(bool enable)
{
    d->sqlResult->setPositionalBindingEnabled(enable);
}

bool AsyncSqlQuery::isPositionalBindingEnabled() const
{
    return d->sqlResult->isPositionalBindingEnabled();
}

void AsyncSqlQuery::finish()
{
    if (isActive()) {
        d->sqlResult->setLastError(QSqlError());
        d->sqlResult->setAt(QSql::BeforeFirstRow);
        d->sqlResult->detachFromResultSet();
        d->sqlResult->setActive(false);
    }
}

Task<bool> AsyncSqlQuery::nextResult()
{
    if (isActive()) {
        co_return co_await d->sqlResult->nextResult();
    }
    co_return false;
}

#include "psql_result.h"
#include "psql_types.h"
#include "psql_driver_p.h"
#include "psql_log.h"
#include "utils.h"

#include "qcoroasyncsqlquery.h"
#include "qcoroasyncsqlresult.h"
#include "qcoroasyncsqlresult_p.h"

#include <QVariant>
#include <QByteArrayView>
#include <QtNumeric>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QSqlRecord>
#include <QSqlField>
#include <atomic>
#include <qtsqlglobal.h>

#include <libpq-fe.h>
#include <postgres_ext.h>

#include <arpa/inet.h>
#include <cstdint>
#include <queue>
#include <vulkan/vulkan_core.h>

using namespace QCoro;

class QCoroAsyncPsqlResultPrivate : public QCoro::AsyncSqlResultPrivate
{
public:
    QCoroAsyncPsqlResultPrivate(QCoroAsyncPsqlResult *qq, const QCoroAsyncPsqlDriver *driver)
        : AsyncSqlResultPrivate(qq, driver)
    {}

    inline QCoroAsyncPsqlDriverPrivate *drv_d_func() {
        return static_cast<QCoroAsyncPsqlDriverPrivate *>(QCoro::AsyncSqlResultPrivate::drv_d_func());
    }
    inline const QCoroAsyncPsqlDriverPrivate *drv_d_func() const {
        return static_cast<const QCoroAsyncPsqlDriverPrivate *>(QCoro::AsyncSqlResultPrivate::drv_d_func());
    }

    Task<void> deallocatePreparedStmt()
    {
        if (const auto d = drv_d_func(); d) {
            const QString query = QStringLiteral("DEALLOCATE %1").arg(preparedStmtId);
            auto result = co_await d->exec(query);

            if (PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
                qCWarning(AsyncPsqlLog, "QCoroAsyncPsqlResult::deallocatePreparedStmt: Unable to deallocate prepared statement: %s", PQerrorMessage(d->conn));
            }
        }
        preparedStmtId.clear();
    }

    Task<bool> processResults()
    {
        Q_Q(QCoroAsyncPsqlResult);
        if (!result) {
            q->setSelect(false);
            q->setActive(false);
            currentSize = -1;
            canFetchMoreRows = false;
            if (stmtId != drv_d_func()->currentStmtId) {
                q->setLastError(makeError(QStringLiteral("QCoroAsyncPSqlResult: Query results lost, probably discarded on executing another query"),
                                            QSqlError::StatementError, drv_d_func()));
                if (drv_d_func()) {
                    drv_d_func()->finishQuery(stmtId);
                }
                stmtId = QCoroPsql::InvalidStatementId;
                co_return false;
            }
        }

        const auto status = PQresultStatus(result.get());
        switch (status) {
        case PGRES_TUPLES_OK:
            q->setActive(true);
            q->setSelect(true);
            currentSize = q->isForwardOnly() ? -1 : PQntuples(result.get());
            canFetchMoreRows = false;
            co_return false;
        case PGRES_SINGLE_TUPLE:
            q->setActive(true);
            q->setSelect(true);
            currentSize = 1;
            canFetchMoreRows = true;
            co_return true;
        case PGRES_COMMAND_OK:
            q->setActive(true);
            q->setSelect(false);
            currentSize = -1;
            canFetchMoreRows = false;
            co_return false;
        default:
            break;
        }

        q->setSelect(false);
        q->setActive(false);
        currentSize = -1;
        canFetchMoreRows = false;
        q->setLastError(makeError(QStringLiteral("QCoroAsyncPSqlResult: Unable to create query"),
                                    QSqlError::StatementError, drv_d_func(), result.get()));
        co_return false;
    }

    QCoroPsql::PGresultPtr result{nullptr, &PQclear};
    std::queue<QCoroPsql::PGresultPtr> nextResultSets;
    ssize_t currentSize = -1;
    bool canFetchMoreRows = false;
    QCoroPsql::StatementId stmtId = QCoroPsql::InvalidStatementId;

    bool preparedQueriesEnabled = true;
    QString preparedStmtId;

private:
    Q_DECLARE_PUBLIC(QCoroAsyncPsqlResult)
};

QCoroAsyncPsqlResult::QCoroAsyncPsqlResult(const QCoroAsyncPsqlDriver *driver)
    : AsyncSqlResult(*new QCoroAsyncPsqlResultPrivate(this, driver))
{}

QCoroAsyncPsqlResult::~QCoroAsyncPsqlResult()
{
    cleanup();
}

QVariant QCoroAsyncPsqlResult::handle() const
{
    Q_D(const QCoroAsyncPsqlResult);
    return QVariant::fromValue(d->result.get());
}

void QCoroAsyncPsqlResult::cleanup()
{
    Q_D(QCoroAsyncPsqlResult);
    if (d->result) {
        d->result.reset();
    }

    if (d->stmtId != QCoroPsql::InvalidStatementId) {
        if (d->drv_d_func()) {
            d->drv_d_func()->finishQuery(d->stmtId);
        }
        d->stmtId = QCoroPsql::InvalidStatementId;
    }

    setAt(QSql::BeforeFirstRow);
    d->currentSize = -1;
    d->canFetchMoreRows = false;
    setActive(false);
}

Task<bool> QCoroAsyncPsqlResult::fetch(int i)
{
    Q_D(QCoroAsyncPsqlResult);
    if (!isActive()) {
        co_return false;
    }
    if (i < 0) {
        co_return false;
    }
    if (at() == i) {
        co_return true;
    }

    if (isForwardOnly()) {
        if (i < at()) {
            co_return false;
        }
        bool ok = true;
        while (ok && i > at()) {
            ok = co_await fetchNext();
        }
        co_return ok;
    }

    if (i >= d->currentSize) {
        co_return false;
    }

    setAt(i);
    co_return true;
}

Task<bool> QCoroAsyncPsqlResult::fetchFirst()
{
    Q_D(const QCoroAsyncPsqlResult);
    if (!isActive()) {
        co_return false;
    }
    if (at() == 0) {
        co_return true;
    }

    if (isForwardOnly()) {
        if (at() == QSql::BeforeFirstRow) {
            if (d->result && PQntuples(d->result.get()) > 0) {
                setAt(0);
                co_return true;
            }
        }
    }

    co_return co_await fetch(0);
}

Task<bool> QCoroAsyncPsqlResult::fetchLast()
{
    Q_D(const QCoroAsyncPsqlResult);
    if (!isActive()) {
        co_return false;
    }

    if (isForwardOnly()) {
        int i = at();
        if (i == QSql::AfterLastRow) {
            co_return false;
        }
        if (i == QSql::BeforeFirstRow) {
            i = 0;
        }
        while (co_await fetchNext()) {
            ++i;
        }
        setAt(i);
        co_return true;
    }

    co_return co_await fetch(d->currentSize - 1);
}

Task<bool> QCoroAsyncPsqlResult::fetchNext()
{
    Q_D(QCoroAsyncPsqlResult);
    if (!isActive()) {
        co_return false;
    }

    const int currentRow = at();
    if (currentRow == QSql::BeforeFirstRow) {
        co_return co_await fetchFirst();
    }
    if (currentRow == QSql::AfterLastRow) {
        co_return false;
    }

    if (isForwardOnly()) {
        if (!d->canFetchMoreRows) {
            co_return false;
        }

        d->result = co_await d->drv_d_func()->getResult(d->stmtId);
        if (!d->result) {
            setLastError(makeError(QStringLiteral("QCoroPSQLResult: Unable to get result"), QSqlError::StatementError, d->drv_d_func(), d->result.get()));
            d->canFetchMoreRows = false;
            co_return false;
        }

        const int status = PQresultStatus(d->result.get());
        switch (status) {
        case PGRES_SINGLE_TUPLE:
            Q_ASSERT(PQntuples(d->result.get()) == 1);
            Q_ASSERT(d->canFetchMoreRows);
            setAt(currentRow + 1);
            co_return true;
        case PGRES_TUPLES_OK:
            Q_ASSERT(PQntuples(d->result.get()) == 0);
            d->canFetchMoreRows = false;
            co_return false;
        default:
            setLastError(makeError(QStringLiteral("QPSQLResult: Unable to get result"), QSqlError::StatementError, d->drv_d_func(), d->result.get()));
            d->canFetchMoreRows = false;
            co_return false;
        }
    }

    if (currentRow + 1 == d->currentSize) {
        co_return false;
    }

    setAt(currentRow + 1);
    co_return true;
}

Task<bool> QCoroAsyncPsqlResult::nextResult()
{
    Q_D(QCoroAsyncPsqlResult);
    if (!isActive()) {
        co_return false;
    }

    setAt(QSql::BeforeFirstRow);

    if (isForwardOnly()) {
        if (d->canFetchMoreRows) {
            while (d->result && PQresultStatus(d->result.get()) == PGRES_SINGLE_TUPLE) {
                d->result = co_await d->drv_d_func()->getResult(d->stmtId);
            }
            d->canFetchMoreRows = false;
            if (d->result && PQresultStatus(d->result.get()) == PGRES_FATAL_ERROR) {
                co_return co_await d->processResults();
            }
        }

        d->result = co_await d->drv_d_func()->getResult(d->stmtId);
        co_return co_await d->processResults();
    }

    if (!d->nextResultSets.empty()) {
        d->result = std::move(d->nextResultSets.front());
        d->nextResultSets.pop();
    }

    co_return co_await d->processResults();
}

namespace {

QVariant doubleFromString(const char *val, ssize_t len, uint ptype, QSql::NumericalPrecisionPolicy precision)
{
    if (ptype == QCoroPsql::NumericOID) {
        if (precision == QSql::HighPrecision) {
            return QString::fromLatin1(val, len);
        }
    }

    char *out = nullptr;
    double dbl = std::strtod(val, &out);
    if (out == val) {
        if (QByteArrayView(val, len).compare("infinity", Qt::CaseInsensitive) == 0) {
            return qInf();
        }
        if (QByteArrayView(val, len).compare("-infinity", Qt::CaseInsensitive) == 0) {
            return -qInf();
        }
        if (QByteArrayView(val, len).compare("nan", Qt::CaseInsensitive) == 0) {
            return qQNaN();
        }
        return {};
    }

    if (ptype == QCoroPsql::NumericOID) {
        if (precision == QSql::LowPrecisionInt64) {
            return static_cast<long long>(dbl);
        }
        if (precision == QSql::LowPrecisionInt32) {
            return static_cast<int>(dbl);
        }
        if (precision == QSql::LowPrecisionDouble) {
            return dbl;
        }
    }

    return dbl;
}

}

QVariant QCoroAsyncPsqlResult::data(int index)
{
    Q_D(const QCoroAsyncPsqlResult);
    if (index >= PQnfields(d->result.get())) {
        qCWarning(AsyncPsqlLog, "QCoroAsyncPsqlResult::data: column %d out of range.", index);
        return {};
    }

    const auto currentRow = isForwardOnly() ? 0 : at();
    const auto ptype = PQftype(d->result.get(), index);
    const auto type = QCoroPsql::decodePsqlType(ptype);

    if (PQgetisnull(d->result.get(), currentRow, index)) {
        return QVariant(type, nullptr);
    }
    const char *value = PQgetvalue(d->result.get(), currentRow, index);
    const auto valLen = PQgetlength(d->result.get(), currentRow, index);
    switch (type.id()) {
    case QMetaType::Bool:
        return QVariant(static_cast<bool>(value[0] == 't'));
    case QMetaType::QString:
        return QString::fromUtf8(value, PQgetlength(d->result.get(), currentRow, index));
    case QMetaType::LongLong:
        if (value[0] == '-') {
            return QByteArrayView(value, valLen).toLongLong();
        } else {
            return QByteArrayView(value, valLen).toULongLong();
        }
    case QMetaType::Int:
        return atoi(value);
    case QMetaType::Double:
        return doubleFromString(value, valLen, ptype, numericalPrecisionPolicy());
    case QMetaType::QDate:
        return QDate::fromString(QLatin1String(value, valLen), Qt::ISODate);
    case QMetaType::QTime:
        return QTime::fromString(QLatin1String(value, valLen), Qt::ISODate);
    case QMetaType::QDateTime: {
        QString tzString = QString::fromLatin1(value, valLen);
        if (!tzString.endsWith(u'Z')) {
            tzString.append(u'Z');
        }
        return QDateTime::fromString(tzString, Qt::ISODate);
    }
    case QMetaType::QByteArray: {
        size_t unescapedLen = 0;
        const std::unique_ptr<unsigned char, decltype(&PQfreemem)> data(PQunescapeBytea(reinterpret_cast<const uchar *>(value), &unescapedLen), &PQfreemem);
        return QByteArray(reinterpret_cast<const char *>(data.get()), unescapedLen);
    }
    default:
        qCWarning(AsyncPsqlLog, "QCoroAsyncPsqlResult::data: unsupported type %d", type.id());
    }

    return {};
}

bool QCoroAsyncPsqlResult::isNull(int field)
{
    Q_D(const QCoroAsyncPsqlResult);
    const auto currentRow = isForwardOnly() ? 0 : at();
    return PQgetisnull(d->result.get(), currentRow, field);
}

QCoro::Task<bool> QCoroAsyncPsqlResult::reset(const QString &query)
{
    Q_D(QCoroAsyncPsqlResult);
    cleanup();
    if (!driver() || !driver()->isOpen() || !driver()->isOpenError()) {
        co_return false;
    }

    d->stmtId = co_await d->drv_d_func()->sendQuery(query);
    if (d->stmtId == QCoroPsql::InvalidStatementId) {
        setLastError(makeError(QStringLiteral("QCoroAsyncPsqlResult::reset: unable to send query"), QSqlError::StatementError, d->drv_d_func()));
        co_return false;
    }

    if (isForwardOnly()) {
        setForwardOnly(d->drv_d_func()->setSingleRowMode());
    }

    d->result = co_await d->drv_d_func()->getResult();
    if (!isForwardOnly()) {
        while (auto nextResult = co_await d->drv_d_func()->getResult(d->stmtId)) {
            d->nextResultSets.push(std::move(nextResult));
        }
    }

    co_return co_await d->processResults();
}

int QCoroAsyncPsqlResult::size()
{
    Q_D(const QCoroAsyncPsqlResult);
    return d->currentSize;
}

int QCoroAsyncPsqlResult::numRowsAffected()
{
    Q_D(const QCoroAsyncPsqlResult);
    const char *tuples = PQcmdTuples(d->result.get());
    return QByteArray::fromRawData(tuples, qstrlen(tuples)).toInt();
}

QCoro::Task<QVariant> QCoroAsyncPsqlResult::lastInsertId() const
{
    Q_D(const QCoroAsyncPsqlResult);
    if (d->drv_d_func()->psqlVersion >= Version(8, 1)) {
        AsyncSqlQuery query(driver()->createResult());
        if (co_await query.exec(QStringLiteral("SELECT lastval();")) && co_await query.next()) {
            co_return query.value(0);
        }
    } else if (isActive()) {
        const auto id = PQoidValue(d->result.get());
        if (id != InvalidOid) {
            co_return id;
        }
    }
    co_return {};
}

Task<QSqlRecord> QCoroAsyncPsqlResult::record() const
{
    Q_D(const QCoroAsyncPsqlResult);
    QSqlRecord record;
    if (!isActive() || !isSelect()) {
        co_return record;
    }

    const auto fieldCount = PQnfields(d->result.get());
    QSqlField field;
    for (int i = 0; i < fieldCount; ++i) {
        field.setName(QString::fromUtf8(PQfname(d->result.get(), i)));
        const auto tableOid = PQftable(d->result.get(), i);
        if (tableOid != InvalidOid && !isForwardOnly()) {
            auto &tableName = d->drv_d_func()->oidToTable[static_cast<int>(tableOid)];
            if (tableName.isEmpty()) {
                AsyncSqlQuery query(driver()->createResult());
                if (co_await query.exec(QStringLiteral("SELECT relname FROM pg_class WHERE pg_class.oid = %1").arg(tableOid)) && co_await query.next()) {
                    tableName = query.value(0).toString();
                }
            }
            field.setTableName(tableName);
        } else {
            field.setTableName({});
        }


        const int ptype = PQftype(d->result.get(), i);
        field.setMetaType(QCoroPsql::decodePsqlType(ptype));
        field.setValue(QVariant(field.metaType()));
        auto size = PQfsize(d->result.get(), i);
        auto precision = PQfmod(d->result.get(), i);

        switch (ptype) {
        case QCoroPsql::TimestampOID:
        case QCoroPsql::TimestampTZOID:
            precision = 3;
            break;
        case QCoroPsql::NumericOID:
            if (precision != -1) {
                size = (precision >> 16);
                precision = ((precision - QCoroPsql::VARHDRSZ) & 0xFFFF);
            }
            break;
        case QCoroPsql::BitOID:
        case QCoroPsql::VarBitOID:
            size = precision;
            precision = -1;
            break;
        default:
            if (size == -1 && precision == QCoroPsql::VARHDRSZ) {
                size = precision - QCoroPsql::VARHDRSZ;
                precision = -1;
            }
        }

        field.setLength(size);
        field.setPrecision(precision);
        record.append(field);
    }

    co_return record;
}

void QCoroAsyncPsqlResult::virtual_hook(int id, void *data)
{
    AsyncSqlResult::virtual_hook(id, data);
}

namespace {

QString generatePreparedStatementId()
{
    constinit static std::atomic<int> id{0};
    return QStringLiteral("qcoro_async_psql_stmt_%1").arg(id.fetch_add(1, std::memory_order_relaxed) + 1, 0, 16);
}

} // namespace

Task<bool> QCoroAsyncPsqlResult::prepare(const QString &query)
{
    Q_D(QCoroAsyncPsqlResult);
    if (d->preparedQueriesEnabled) {
        co_return co_await AsyncSqlResult::prepare(query);
    }

    cleanup();

    if (!d->preparedStmtId.isEmpty()) {
        co_await d->deallocatePreparedStmt();
    }

    const auto stmtId = generatePreparedStatementId();
    const QString stmt = QStringLiteral("PREPARE %1 AS %2").arg(stmtId, d->positionalToNamedBinding(query));

    auto result = co_await d->drv_d_func()->exec(stmt);
    if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
        setLastError(makeError(QStringLiteral("QCoroAsyncPsqlResult::prepare: Unable to prepare statement"), QSqlError::StatementError, d->drv_d_func(), result.get()));
        d->preparedStmtId.clear();
        co_return false;
    }

    d->preparedStmtId = stmtId;
    co_return true;
}

namespace {

QString createParamString(const QVariantList &boundValues, const AsyncSqlDriver *driver)
{
    if (boundValues.empty()) {
        return {};
    }

    QString params;
    QSqlField field;
    for (const auto &val : boundValues) {
        field.setMetaType(val.metaType());
        if (AsyncSqlResultPrivate::isVariantNull(val)) {
            field.clear();
        } else {
            field.setValue(val);
        }
        if (!params.isNull()) {
            params.append(u", ");
        }
        params.append(driver->formatValue(field));
    }

    return params;
}

} // namespace

Task<bool> QCoroAsyncPsqlResult::exec()
{
    Q_D(QCoroAsyncPsqlResult);
    if (!d->preparedQueriesEnabled) {
        co_return co_await AsyncSqlResult::exec();
    }

    cleanup();

    const auto params = createParamString(boundValues(), driver());
    const QString stmt = params.isEmpty() ? QStringLiteral("EXECUTE %1").arg(d->preparedStmtId)
                                          : QStringLiteral("EXECUTE %1 (%2)").arg(d->preparedStmtId, params);

    d->stmtId = co_await d->drv_d_func()->sendQuery(stmt);
    if (d->stmtId == QCoroPsql::InvalidStatementId) {
        setLastError(makeError(QStringLiteral("QCoroAsyncPsqlResult::exec: Unable to send query"), QSqlError::StatementError, d->drv_d_func()));
        co_return false;
    }

    if (isForwardOnly()) {
        setForwardOnly(d->drv_d_func()->setSingleRowMode());
    }

    d->result = co_await d->drv_d_func()->getResult(d->stmtId);
    if (!isForwardOnly()) {
        while (auto result = co_await d->drv_d_func()->getResult(d->stmtId)) {
            d->nextResultSets.push(std::move(result));
        }
    }

    co_return co_await d->processResults();
}
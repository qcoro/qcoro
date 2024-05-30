#include "psql_driver.h"
#include "psql_driver_p.h"
#include "psql_types.h"
#include "psql_result.h"
#include "psql_log.h"
#include "utils.h"
#include "qcoroasyncsqldriver.h"
#include "qcoroasyncsqlquery.h"

#include "qcorosocketnotifier.h"
#include "qcorosignal.h"

#include <QSocketNotifier>
#include <QUrl>
#include <QUrlQuery>
#include <QScopeGuard>
#include <QDateTime>

#include <QSqlDriver>
#include <QSqlIndex>
#include <QSqlField>

#include <libpq-fe.h>
#include <qnamespace.h>

using namespace QCoro;
using namespace Qt::StringLiterals;

Q_DECLARE_OPAQUE_POINTER(pg_conn *)
Q_DECLARE_METATYPE(pg_conn *)

class ReadWriteSocketNotifier : public QObject
{
public:
    ReadWriteSocketNotifier(QSocketDescriptor socket, QObject *parent = nullptr)
        : QObject(parent)
        , m_readNotifier(socket, QSocketNotifier::Read)
        , m_writeNotifier(socket, QSocketNotifier::Write)
    {
        m_readNotifier.setEnabled(false);
        m_writeNotifier.setEnabled(false);
        connect(&m_readNotifier, &QSocketNotifier::activated, this, &ReadWriteSocketNotifier::activated);
        connect(&m_writeNotifier, &QSocketNotifier::activated, this, &ReadWriteSocketNotifier::activated);
    }

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> awaiter) noexcept
    {
        m_awaiter = awaiter;
        m_readNotifier.setEnabled(true);
        m_writeNotifier.setEnabled(true);
    }

    QSocketNotifier::Type await_resume() const noexcept
    {
        Q_ASSERT(m_triggeredType.has_value());
        return *m_triggeredType;
    }

private Q_SLOTS:
    void activated([[maybe_unused]] QSocketDescriptor socket, QSocketNotifier::Type type)
    {
        m_triggeredType = type;
        m_readNotifier.setEnabled(false);
        m_writeNotifier.setEnabled(false);
        m_awaiter.resume();
    }

private:
    QSocketNotifier m_readNotifier;
    QSocketNotifier m_writeNotifier;
    std::coroutine_handle<> m_awaiter;
    std::optional<QSocketNotifier::Type> m_triggeredType;
};

QCoroAsyncPsqlDriverPrivate::QCoroAsyncPsqlDriverPrivate()
    : AsyncSqlDriverPrivate(AsyncSqlDriver::PostgreSQL)
{
}

Task<void> QCoroAsyncPsqlDriverPrivate::appendTables(QStringList &tables, AsyncSqlQuery &query, QChar type)
{
    const QString stmt =
            QStringLiteral("SELECT pg_class.relname, pg_namespace.nspname "
                           "FROM pg_class "
                           "LEFT JOIN pg_namespace ON (pg_class.relnamespace = pg_namespace.oid) "
                           "WHERE (pg_class.relkind = '%1')"
                            "     AND (pg_class.relname !~ '^Inv')"
                            "     AND (pg_class.relname !~ '^pg_')"
                            "     AND (pg_namespace.nspname != 'information_schema')").arg(type);

    co_await query.exec(stmt);
    while (co_await query.next()) {
        const auto schema = query.value(1).toString();
        if (schema.isEmpty() || schema == "public"_L1) {
            tables.append(query.value(0).toString());
        } else {
            tables.append(query.value(0).toString().prepend(u'.').prepend(schema));
        }
    }
}

Task<QCoroPsql::PGresultPtr> QCoroAsyncPsqlDriverPrivate::exec(const QString &query)
{
    return exec(query.toUtf8().constData());
}

Task<QCoroPsql::PGresultPtr> QCoroAsyncPsqlDriverPrivate::exec(const char *query)
{
    if (const auto status = PQsendQuery(conn, query); status == 0) {
        qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to send query"), QSqlError::StatementError, this);
        co_return QCoroPsql::makePGResultPtr(nullptr);
    }

    while (true) {
        switch (PQflush(conn)) {
        case 0: // the entire query has been written to server
            goto query_flushed;
        case 1: // there are more data to be sent - wait for the socket to become writable again
            switch (co_await ReadWriteSocketNotifier(PQsocket(conn))) {
            case QSocketNotifier::Read:
                PQconsumeInput(conn); // consume incoming data from socket
                continue;             // and try to write the outgoing data again
            case QSocketNotifier::Write:
                continue;             // socket ready for writing, try to write the rest of the data
            case QSocketNotifier::Exception:
                qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to send query"), QSqlError::StatementError, this);
                co_return QCoroPsql::makePGResultPtr(nullptr);
            }
            Q_UNREACHABLE();
        case -1: // sending query to server failed
            qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to send query"), QSqlError::StatementError, this);
            co_return QCoroPsql::makePGResultPtr(nullptr);
        }
    }

    query_flushed:

    QCoroPsql::PGresultPtr lastResult{nullptr, &PQclear};
    QCoroPsql::PGresultPtr result{nullptr, &PQclear};
    while ((result = co_await getResult())) {
        lastResult = std::move(result);

        // If the connection to the server is broken, stop, otherwise we would never leave the loop
        if (PQstatus(conn) == CONNECTION_BAD) {
            break;
        }

        const auto status = PQresultStatus(lastResult.get());
        if (status == PGRES_COPY_IN || status == PGRES_COPY_OUT || status == PGRES_COPY_BOTH) {
            break;
        }
    }

    co_return std::move(lastResult);
}

Task<QCoroPsql::StatementId> QCoroAsyncPsqlDriverPrivate::sendQuery(const QString &stmt)
{
    discardResults();
    if (const auto result = PQsendQuery(conn, stmt.toUtf8().constData()); result == 1) {
        setLastError(makeError(QStringLiteral("Unable to send query"), QSqlError::StatementError, this));
        co_return QCoroPsql::InvalidStatementId;
    }

    while (true) {
        switch (PQflush(conn)) {
        case 0: // the entire query has been written to server
            currentStmtId = generateStatementId();
            co_return currentStmtId;
        case 1: // there are more data to be sent - wait for the socket to become writable again
            switch (co_await ReadWriteSocketNotifier(PQsocket(conn))) {
            case QSocketNotifier::Read:
                PQconsumeInput(conn); // consume incoming data from socket
                continue;             // and try to write the outgoing data again
            case QSocketNotifier::Write:
                continue;             // socket ready for writing, try to write the rest of the data
            case QSocketNotifier::Exception:
                setLastError(makeError(QStringLiteral("Unable to send query"), QSqlError::StatementError, this));
                currentStmtId = QCoroPsql::InvalidStatementId;
                co_return currentStmtId;
            }
            Q_UNREACHABLE();
        case -1: // sending query to server failed
            setLastError(makeError(QStringLiteral("Unable to send query"), QSqlError::StatementError, this));
            currentStmtId = QCoroPsql::InvalidStatementId;
            co_return currentStmtId;
        }
    }

    Q_UNREACHABLE();
}

bool QCoroAsyncPsqlDriverPrivate::setSingleRowMode() const
{
#if defined PG_VERSION_NUM && PG_VERSION_NUM-0 >= 90200
    return PQsetSingleRowMode(conn) == 1;
#else
    return false;
#endif
}

Task<QCoroPsql::PGresultPtr> QCoroAsyncPsqlDriverPrivate::getResult() const
{
    while (true) {
        // Consume whatever data are waiting in the socket
        if (const auto result = PQconsumeInput(conn); result == 0) {
            setLastError(makeError(QStringLiteral("Unable to fetch query result"), QSqlError::StatementError, this));
            co_return QCoroPsql::makePGResultPtr(nullptr);
        }

        // Check whether we have enough data buffered locally to produce a result, if not (result == 1),
        // wait for the socket to receive more data and restart the loop
        if (const auto result = PQisBusy(conn); result == 1) {
            co_await QSocketNotifier(PQsocket(conn), QSocketNotifier::Read);
            continue;
        }

        co_return QCoroPsql::makePGResultPtr(PQgetResult(conn));
    }
}

Task<QCoroPsql::PGresultPtr> QCoroAsyncPsqlDriverPrivate::getResult(QCoroPsql::StatementId stmtId) const
{
    if (stmtId != currentStmtId) {
        qCWarning(AsyncPsqlLog) << "QCoroAsyncPsqlDriver::getResult: Query results lost - probably due to a new query being sent";
        co_return QCoroPsql::makePGResultPtr(nullptr);
    }

    co_return co_await getResult();
}

void QCoroAsyncPsqlDriverPrivate::finishQuery(QCoroPsql::StatementId stmtId)
{
    if (stmtId == currentStmtId && stmtId != QCoroPsql::InvalidStatementId) {
        discardResults();
        currentStmtId = QCoroPsql::InvalidStatementId;
    }
}

QCoroPsql::StatementId QCoroAsyncPsqlDriverPrivate::generateStatementId()
{
    auto stmtId = ++stmtCount;
    if (stmtId <= 0) {
        stmtId = 1;
    }
    return stmtId;
}

bool QCoroAsyncPsqlDriverPrivate::setNonblockingConnection()
{
    return PQsetnonblocking(conn, 1) == 0;
}

bool QCoroAsyncPsqlDriverPrivate::detectServerVersion()
{
    const auto version = PQserverVersion(conn);
    if (version == 0) {
        setLastError(makeError(QStringLiteral("Unable to detect protocol version"), QSqlError::ConnectionError, this));
        return false;
    }

    const auto majorMinor = (version / 100);
    psqlVersion = Version{majorMinor / 100, majorMinor % 100};

    return true;
}

Task<bool> QCoroAsyncPsqlDriverPrivate::setEncodingUtf8()
{

    const auto result = co_await exec("SET client_encoding TO 'UTF8'");
    if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
        qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to set client encoding to UTF-8"), QSqlError::ConnectionError, this, result.get());
        co_return false;
    }

    co_return true;
}

Task<bool> QCoroAsyncPsqlDriverPrivate::setDateStyle()
{
    const auto result = co_await exec("SET datestyle TO 'ISO'");
    if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
        qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to set client encoding to UTF-8"), QSqlError::ConnectionError, this, result.get());
        co_return false;
    }

    co_return true;
}

Task<bool> QCoroAsyncPsqlDriverPrivate::setByteaOutput()
{
    // Use legacy 'escape' format (new versions default to hex)
    if (psqlVersion >= Version{9, 0}) {
        const auto result = co_await exec("SET bytea_output TO 'escape'");
        if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
            qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to set bytea output to escape"), QSqlError::ConnectionError, this, result.get());
            co_return false;
        }
    }

    co_return true;
}

Task<bool> QCoroAsyncPsqlDriverPrivate::setUtcTimeZone()
{
    const auto result = co_await exec("SET timezone TO 'UTC'");
    if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
        qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to set timezone to UTC"), QSqlError::ConnectionError, this, result.get());
        co_return false;
    }

    co_return true;
}

Task<bool> QCoroAsyncPsqlDriverPrivate::detectBackslashEscape()
{
    if (psqlVersion <= Version{8, 2}) {
        hasBackslashEscape = true;
    } else {
        hasBackslashEscape = false;
        const auto result = co_await exec("SELECT '\\\\' x");
        if (!result || (PQresultStatus(result.get()) != PGRES_COMMAND_OK && PQresultStatus(result.get()) != PGRES_TUPLES_OK)) {
            qCWarning(AsyncPsqlLog) << makeError(QStringLiteral("Unable to detect backslash escape"), QSqlError::ConnectionError, this, result.get());
            co_return false;
        }

        hasBackslashEscape = QString::fromLatin1(PQgetvalue(result.get(), 0, 0)) == "\\"_L1;
    }

    co_return true;
}

QCoroAsyncPsqlDriver::QCoroAsyncPsqlDriver(QObject *parent)
    : AsyncSqlDriver(new QCoroAsyncPsqlDriverPrivate(), parent)
{
}

QCoroAsyncPsqlDriver::~QCoroAsyncPsqlDriver()
{
    Q_D(QCoroAsyncPsqlDriver);
    PQfinish(d->conn);
}

QVariant QCoroAsyncPsqlDriver::handle() const
{
    Q_D(const QCoroAsyncPsqlDriver);
    return QVariant::fromValue(d->conn);
}

bool QCoroAsyncPsqlDriver::hasFeature(DriverFeature feature) const
{
    Q_D(const QCoroAsyncPsqlDriver);
    switch (feature) {
    case QSqlDriver::Transactions:
    case QSqlDriver::QuerySize:
    case QSqlDriver::LastInsertId:
    case QSqlDriver::LowPrecisionNumbers:
    case QSqlDriver::MultipleResultSets:
    case QSqlDriver::BLOB:
    case QSqlDriver::Unicode:
        return true;
    case QSqlDriver::PreparedQueries:
    case QSqlDriver::PositionalPlaceholders:
        return d->psqlVersion >= Version{8, 2};
    case QSqlDriver::EventNotifications:
    case QSqlDriver::BatchOperations:
    case QSqlDriver::NamedPlaceholders:
    case QSqlDriver::SimpleLocking:
    case QSqlDriver::FinishQuery:
    case QSqlDriver::CancelQuery:
        return false;
    }
}

bool QCoroAsyncPsqlDriver::isOpen() const
{
    Q_D(const QCoroAsyncPsqlDriver);
    return PQstatus(d->conn) == CONNECTION_OK;
}

namespace {

QString buildConnInfo(const QString &db, const QString &user, const QString &password,
                      const QString &host, int port, const QString &connOpts)
{
    const auto quote = [](QString str) {
        str.replace(u'\\', "\\\\"_L1);
        str.replace(u'\'', "\\'"_L1);
        str.prepend(u'\'');
        str.append(u'\'');
        return str;
    };

    QString connString;
    if (!host.isEmpty()) {
        connString += QStringLiteral("host=%1").arg(quote(host));
    }
    if (!db.isEmpty()) {
        connString += QStringLiteral(" dbname=%1").arg(quote(db));
    }
    if (!user.isEmpty()) {
        connString += QStringLiteral(" user=%1").arg(quote(user));
    }
    if (!password.isEmpty()) {
        connString += QStringLiteral(" password=%1").arg(quote(password));
    }
    if (port != -1) {
        connString += QStringLiteral(" port=%1").arg(port);
    }

    if (!connOpts.isEmpty()) {
        QString opt = connOpts;
        opt.replace(';'_L1, ' '_L1, Qt::CaseInsensitive);
        connString.append(opt);
    }

    return connString;
}

} // namespace

Task<bool> QCoroAsyncPsqlDriver::open(const QString &db, const QString &user, const QString &password,
                                      const QString &host, int port, const QString &connOpts)
{
    Q_D(QCoroAsyncPsqlDriver);
    const auto connInfo = buildConnInfo(db, user, password, host, port, connOpts);
    d->conn = PQconnectStart(connInfo.toUtf8().constData());
    if (!d->conn) {
        setLastError(makeError(QStringLiteral("Unable to connect"), QSqlError::ConnectionError, d));
        setOpenError(true);
        co_return false;
    }

    auto cleanupConn = qScopeGuard([d]() {
        PQfinish(d->conn);
        d->conn = nullptr;
    });

    Q_FOREVER {
        co_await QSocketNotifier(PQsocket(d->conn), QSocketNotifier::Write);
        switch (PQconnectPoll(d->conn)) {
        case PGRES_POLLING_READING:
            co_await QSocketNotifier(PQsocket(d->conn), QSocketNotifier::Read);
            continue;
        case PGRES_POLLING_WRITING:
            co_await QSocketNotifier(PQsocket(d->conn), QSocketNotifier::Write);
            continue;
        case PGRES_POLLING_FAILED:
            setLastError(makeError(QStringLiteral("Unable to connect"), QSqlError::ConnectionError, d));
            setOpenError(true);
            co_return false;
        case PGRES_POLLING_OK:
            goto connected;
        case PGRES_POLLING_ACTIVE:
            // should be unused, per documentation
            Q_ASSERT_X(false, Q_FUNC_INFO, "PGRES_POLLING_ACTIVE should not be returned by PQconnectPoll");
            setLastError(makeError(QStringLiteral("Unable to connect"), QSqlError::ConnectionError, d));
            setOpenError(true);
            co_return false;
        }
    }
    connected:

    d->setNonblockingConnection();
    d->detectServerVersion();
    co_await d->detectBackslashEscape();
    if (!co_await d->setEncodingUtf8()) {
        setLastError(makeError(QStringLiteral("Unable to set client encoding to UTF-8"), QSqlError::ConnectionError, d));
        setOpenError(true);
        co_return false;
    }
    co_await d->setDateStyle();
    co_await d->setByteaOutput();
    co_await d->setUtcTimeZone();

    cleanupConn.dismiss();

    setOpen(true);
    setOpenError(false);
    co_return true;
}

Task<void> QCoroAsyncPsqlDriver::close()
{
    Q_D(QCoroAsyncPsqlDriver);

    PQfinish(d->conn);
    d->conn = nullptr;
    setOpen(false);
    setOpenError(false);

    co_return;
}

AsyncSqlResult *QCoroAsyncPsqlDriver::createResult() const
{
    return new QCoroAsyncPsqlResult(this);
}

Task<bool> QCoroAsyncPsqlDriver::beginTransaction()
{
    Q_D(QCoroAsyncPsqlDriver);
    if (!isOpen()) {
        qCWarning(AsyncPsqlLog) << "QCoroAsyncPsqlDriver: Unable to begin transaction: database not open";
        co_return false;
    }

    auto result = co_await d->exec("BEGIN");
    if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
        setLastError(makeError(QStringLiteral("Unable to begin transaction"), QSqlError::TransactionError, d, result.get()));
        co_return false;
    }

    co_return true;
}

Task<bool> QCoroAsyncPsqlDriver::commitTransaction()
{
    Q_D(QCoroAsyncPsqlDriver);
    if (!isOpen()) {
        qCWarning(AsyncPsqlLog) << "QCoroAsyncPsqlDriver: Unable to commit transaction: database not open";
        co_return false;
    }

    auto result = co_await d->exec("COMMIT");

    bool transaction_failed = false;

    if (d->psqlVersion >= Version{8, 0}) {
        transaction_failed = qstrncmp(PQcmdStatus(result.get()), "ROLLBACK", 8) == 0;
    }

    if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK || transaction_failed) {
        setLastError(makeError(QStringLiteral("Unable to commit transaction"), QSqlError::TransactionError, d, result.get()));
        co_return false;
    }

    co_return true;
}

Task<bool> QCoroAsyncPsqlDriver::rollbackTransaction()
{
    Q_D(QCoroAsyncPsqlDriver);
    if (!isOpen()) {
        qCWarning(AsyncPsqlLog) << "QCoroAsyncPsqlDriver: Unable to rollback transaction: database not open";
        co_return false;
    }

    auto result = co_await d->exec("ROLLBACK");

    if (!result || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
        setLastError(makeError(QStringLiteral("Unable to rollback transaction"), QSqlError::TransactionError, d, result.get()));
        co_return false;
    }

    co_return true;
}

Task<QStringList> QCoroAsyncPsqlDriver::tables(QSql::TableType type) const
{
    Q_D(const QCoroAsyncPsqlDriver);
    QStringList tables;
    if (!isOpen()) {
        co_return tables;
    }

    AsyncSqlQuery query(createResult());
    query.setForwardOnly(true);

    if (type & QSql::Tables) {
        co_await const_cast<QCoroAsyncPsqlDriverPrivate *>(d)->appendTables(tables, query, u'r');
    }
    if (type & QSql::Views) {
        co_await const_cast<QCoroAsyncPsqlDriverPrivate *>(d)->appendTables(tables, query, u'v');
    }
    if (type & QSql::SystemTables) {
        co_await query.exec(QStringLiteral("SELECT relname FROM pg_class WHERE (relkind = 'r') AND (relname LIKE 'pg_%')"));
        while (co_await query.next()) {
            tables.push_back(query.value(0).toString());
        }
    }

    co_return tables;
}

namespace {

std::tuple<QStringView, QStringView> splitTableName(QStringView tableName)
{
    const auto dot = tableName.indexOf(QLatin1Char('.'));
    if (dot == -1) {
        return {QStringView(), tableName};
    }

    return std::make_tuple(tableName.left(dot), tableName.mid(dot + 1));
}

} // namespace

Task<QSqlIndex> QCoroAsyncPsqlDriver::primaryIndex(const QString &fqTableName) const
{
    QSqlIndex index(fqTableName);
    if (!isOpen()) {
        co_return index;
    }

    AsyncSqlQuery query(createResult());

    auto [schemaName, tableName] = splitTableName(fqTableName);
    schemaName = stripDelimiters(schemaName, QSqlDriver::TableName);
    tableName = stripDelimiters(tableName, QSqlDriver::TableName);

    QString stmt = QStringLiteral("SELECT pg_attribute.attname, pg_attribute.atttypid::int, "
                                  "       pg_class.relname "
                                  "FROM pg_attribute, pg_class "
                                  "WHERE "
                                  "      %1 "
                                  "      pg_class.iod IN ("
                                  "          SELECT indexrelid FROM pg_index WHERE indisprimary = true AND indrelid IN ("
                                  "              SELECT oid FROM pg_class WHERE relname = '%2'"
                                  "          )"
                                  "      )"
                                  "      AND pg_attribute.attrelid = pg_class.oid "
                                  "      AND pg_attribute.attisdropped = false "
                                  "ORDER BY pg_attribute.attnum");
    if (schemaName.isEmpty()) {
        stmt = stmt.arg(QStringLiteral("pg_table_is_visible(pg_class.oid) AND"));
    } else {
        stmt = stmt.arg(QStringLiteral("pg_class.relnamespace = (SELECT oid FROM pg_namespace WHERE pg_namespace.nspname = '%1') AND").arg(schemaName));
    }

    co_await query.exec(stmt.arg(tableName));
    while (query.isActive() && co_await query.next()) {
        QSqlField field(query.value(0).toString(), QCoroPsql::decodePsqlType(query.value(1).toInt()), tableName.toString());
        index.append(field);
        index.setName(query.value(2).toString());
    }

    co_return index;
}

Task<QSqlRecord> QCoroAsyncPsqlDriver::record(const QString &fqTableName) const
{
    Q_D(const QCoroAsyncPsqlDriver);

    QSqlRecord record;
    if (!isOpen()) {
        co_return record;
    }

    auto [schemaName, tableName] = splitTableName(fqTableName);
    schemaName = stripDelimiters(schemaName, QSqlDriver::TableName);
    tableName = stripDelimiters(tableName, QSqlDriver::TableName);

    const QString adsrc = d->psqlVersion < Version{8, 0}
        ? QStringLiteral("pg_attrdef.adsrc")
        : QStringLiteral("pg_get_expr(pg_attrdef.adbin, pg_attrdef.adrelid)");
    const QString nspname = schemaName.isEmpty()
        ? QStringLiteral("pg_table_is_visible(pg_class.oid)")
        : QStringLiteral("pg_class.relnamespace = (SELECT oid FROM "
                         "pg_namespace WHERE pg_namespace.nspname = '%1')").arg(schemaName);
    const QString stmt =
        QStringLiteral("SELECT pg_attribute.attname, pg_attribute.atttypid::int, "
                       "       pg_attribute.attnotnull, pg_attribute.attlen, pg_attribute.atttypmod, "
                       "       %1 "
                       "FROM pg_class, pg_attribute "
                       "LEFT JOIN pg_attrdef ON (pg_attrdef.adrelid = pg_attribute.attrelid AND pg_attrdef.adnum = pg_attribute.attnum) "
                       "WHERE %2 "
                       "      AND pg_class.relname = '%3' "
                       "      AND pg_attribute.attnum > 0 "
                       "      AND pg_attribute.attrelid = pg_class.oid "
                       "      AND pg_attribute.attisdropped = false "
                       "ORDER BY pg_attribute.attnum").arg(adsrc, nspname, tableName);

    AsyncSqlQuery query(createResult());
    co_await query.exec(stmt);
    while (co_await query.next()) {
        int attLen = query.value(3).toInt();
        int attTypMod = query.value(4).toInt();
        // swap length and precision if length == -1
        if (attLen == -1 && attTypMod > -1) {
            attLen = attTypMod - 4;
            attTypMod = -1;
        }
        QString defVal = query.value(5).toString();
        if (!defVal.isEmpty() && defVal.at(0) == u'\'') {
            const qsizetype end = defVal.lastIndexOf(u'\'');
            if (end > 0)
                defVal = defVal.mid(1, end - 1);
        }
        QSqlField field(query.value(0).toString(), QCoroPsql::decodePsqlType(query.value(1).toInt()), tableName.toString());
        field.setRequired(query.value(2).toBool());
        field.setLength(attLen);
        field.setPrecision(attTypMod);
        field.setDefaultValue(defVal);
        record.append(field);
    }

    co_return record;
}

template<typename FloatType>
QString assignSpecialPsqlFloatValue(FloatType val)
{
    if (qIsNaN(val)) {
        return QStringLiteral("'NaN");
    } else if (qIsInf(val)) {
        return val > 0 ? QStringLiteral("'Infinity'") : QStringLiteral("'-Infinity'");
    }

    return {};
}

QString QCoroAsyncPsqlDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    Q_D(const QCoroAsyncPsqlDriver);
    const auto nullStr = [](){ return QStringLiteral("NULL"); };
    if (field.isNull()) {
        return nullStr();
    }

    switch (field.metaType().id()) {
    case QMetaType::QDateTime: {
        const auto dt = field.value().toDateTime();
        if (dt.isValid()) {
            // we force the value to be considered with a timezone information, and we force it to be UTC
            // this is safe since postgresql stores only the UTC value and not the timezone offset (only used
            // while parsing), so we have correct behavior in both case of with timezone and without tz
            return QStringLiteral("TIMESTAMP WITH TIME ZONE ") + u'\'' +
                    QLocale::c().toString(dt.toUTC(), u"yyyy-MM-ddThh:mm:ss.zzz") +
                    u'Z' + u'\'';
        }
        return nullStr();
    }
    case QMetaType::QTime: {
        const auto time = field.value().toTime();
        if (time.isValid()) {
            return u'\'' + QLocale::c().toString(time, u"hh:mm:ss.zzz") + u'\'';
        }
        return nullStr();
    }
    case QMetaType::QString: {
        auto result = AsyncSqlDriver::formatValue(field, trimStrings);
        if (d->hasBackslashEscape) {
            result.replace(u'\\', "\\\\"_L1);
        }
        return result;
    }
    case QMetaType::Bool:
        return (field.value().toBool()) ? QStringLiteral("TRUE") : QStringLiteral("FALSE");
    case QMetaType::QByteArray: {
        QByteArray ba(field.value().toByteArray());
        size_t len;
        std::unique_ptr<unsigned char, decltype(&PQfreemem)> data(
#if defined PG_VERSION_NUM && PG_VERSION_NUM-0 >= 80200
            PQescapeByteaConn(d->connection, static_cast<const unsigned char *>(ba.constData()), ba.size(), &len),
#else
            PQescapeBytea(reinterpret_cast<const unsigned char *>(ba.constData()), ba.size(), &len),
#endif
            &PQfreemem);
        return u'\''+ QLatin1StringView(reinterpret_cast<const char *>(data.get())) + u'\'';
    }
    case QMetaType::Float: {
        const auto result = assignSpecialPsqlFloatValue(field.value().toFloat());
        return result.isEmpty() ? AsyncSqlDriver::formatValue(field, trimStrings) : result;
    }
    case QMetaType::Double: {
        const auto result = assignSpecialPsqlFloatValue(field.value().toDouble());
        return result.isEmpty() ? AsyncSqlDriver::formatValue(field, trimStrings) : result;
    }
    case QMetaType::QUuid:
        return u'\'' + field.value().toString() + u'\'';
    default:
        return AsyncSqlDriver::formatValue(field, trimStrings);
    }
}

QString QCoroAsyncPsqlDriver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    QString res = identifier;
    if (!identifier.isEmpty() && !identifier.startsWith(u'"') && !identifier.endsWith(u'"') ) {
        res.replace(u'"', "\"\""_L1);
        res.replace(u'.', "\".\""_L1);
        res = u'"' + res + u'"';
    }
    return res;
}


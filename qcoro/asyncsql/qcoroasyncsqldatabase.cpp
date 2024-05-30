// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
// SPDX-FileCopyrightText: 2016 The Qt Company Ltd.
//
// SPDX-License-Identifier: MIT

#include "qcoroasyncsqldatabase.h"
#include "qcoroasyncsqldriver.h"
#include "qcoroasyncsqlnulldriver_p.h"
#include "qcoroasyncsqldriverplugin_p.h"
#include "qcoroasyncsql_log.h"
#include "driverpluginloader_p.h"

#include <QCoreApplication>
#include <QGlobalStatic>
#include <qapplicationstatic.h>
#include <QHash>
#include <QReadWriteLock>
#include <QAtomicInt>
#include <QThread>
#include <QSqlError>

using namespace QCoro;
using namespace Qt::StringLiterals;

#define CHECK_QCOREAPPLICATION(...) \
    if (Q_UNLIKELY(!QCoreApplication::instance())) { \
        qCWarning(AsyncSqlLog, "AsyncSqlDatabase requires a QCoreApplication"); \
        return __VA_ARGS__; \
    }

#define CHECK_QCOREAPPLICATION_CORO(...) \
    if (Q_UNLIKELY(!QCoreApplication::instance())) { \
        qCWarning(AsyncSqlLog, "AsyncSqlDatabase requires a QCoreApplication"); \
        co_return __VA_ARGS__; \
    }

Q_GLOBAL_STATIC_WITH_ARGS(DriverPluginLoader, loader, (QCoroAsyncSqlDriverPlugin_iid, "/qcoro/asyncsqldrivers"_L1))

const char *AsyncSqlDatabase::defaultConnection = "qt_sql_default_connection";

namespace {

class Registry {
    Q_DISABLE_COPY_MOVE(Registry)
public:
    explicit Registry() = default;
    ~Registry();

    AsyncSqlDatabase connection(const QString &key) const
    {
        const QReadLocker locker(&lock);
        return connections.value(key);
    }
    bool connectionExists(const QString &key) const
    {
        const QReadLocker locker(&lock);
        return connections.contains(key);
    }
    QStringList connectionNames() const
    {
        const QReadLocker locker(&lock);
        return connections.keys();
    }
    mutable QReadWriteLock lock;
    QHash<QString, AsyncSqlDriverCreatorBase*> registeredDrivers;
    QHash<QString, AsyncSqlDatabase> connections;
};

} // namespace

Q_APPLICATION_STATIC(Registry, s_registry);

namespace QCoro
{

class AsyncSqlDatabasePrivate
{
public:
    explicit AsyncSqlDatabasePrivate(AsyncSqlDriver *dr)
        : driver(dr)
    {}

    AsyncSqlDatabasePrivate(const AsyncSqlDatabasePrivate &other);
    AsyncSqlDatabasePrivate(AsyncSqlDatabasePrivate &&) noexcept = delete;
    ~AsyncSqlDatabasePrivate();

    AsyncSqlDatabasePrivate &operator=(const AsyncSqlDatabasePrivate &) = delete;
    AsyncSqlDatabasePrivate &operator=(const AsyncSqlDatabasePrivate &&) noexcept = delete;

    void init(const QString& type);
    void copy(const AsyncSqlDatabasePrivate *other);
    void disable();

    QAtomicInt ref = 1;
    AsyncSqlDriver* driver;
    QString dbname;
    QString userName;
    QString password;
    QString hostname;
    QString drvName;
    int port = -1;
    QString connOptions;
    QString connName;
    QSql::NumericalPrecisionPolicy precisionPolicy = QSql::LowPrecisionDouble;

    static AsyncSqlDatabasePrivate *shared_null();
    static Task<AsyncSqlDatabase> database(const QString& name, bool open);
    static void addDatabase(const AsyncSqlDatabase &db, const QString & name);
    static void removeDatabase(const QString& name);
    static void invalidateDb(const AsyncSqlDatabase &db, const QString &name, bool doWarn = true);
};

} // namespace QCoro

Registry::~Registry()
{
    qDeleteAll(registeredDrivers);
    for (const auto &[k, v] : std::as_const(connections).asKeyValueRange()) {
        AsyncSqlDatabasePrivate::invalidateDb(v, k, false);
    }
}

AsyncSqlDatabasePrivate::AsyncSqlDatabasePrivate(const AsyncSqlDatabasePrivate &other)
    : ref(1)
    , driver(other.driver)
{
    copy(&other);
}

void AsyncSqlDatabasePrivate::copy(const AsyncSqlDatabasePrivate *other)
{
    dbname = other->dbname;
    userName = other->userName;
    password = other->password;
    hostname = other->hostname;
    drvName = other->drvName;
    port = other->port;
    connOptions = other->connOptions;
    precisionPolicy = other->precisionPolicy;
    if (driver) {
        driver->setNumericalPrecisionPolicy(other->driver->numericalPrecisionPolicy());
    }
}

AsyncSqlDatabasePrivate::~AsyncSqlDatabasePrivate()
{
    if (driver != shared_null()->driver) {
        delete driver;
    }
}

AsyncSqlDatabasePrivate *AsyncSqlDatabasePrivate::shared_null()
{
    static AsyncSqlNullDriver driver;
    static AsyncSqlDatabasePrivate db(&driver);
    return &db;
}

void AsyncSqlDatabasePrivate::invalidateDb(const AsyncSqlDatabase &db, const QString &name, bool doWarn)
{
    if (db.d->ref.loadRelaxed() != 1 && doWarn) {
        qCWarning(AsyncSqlLog, "AsyncSqlDatabasePrivate::removeDatabase: connection '%ls' is still in use, "
                 "all queries will cease to work.", qUtf16Printable(name));
        db.d->disable();
        db.d->connName.clear();
    }
}

void AsyncSqlDatabasePrivate::removeDatabase(const QString &name)
{
    CHECK_QCOREAPPLICATION()

    auto *registry = s_registry();
    QWriteLocker locker(&registry->lock);

    if (!registry->connections.contains(name)) {
        return;
    }

    invalidateDb(registry->connections.take(name), name);
}

void AsyncSqlDatabasePrivate::addDatabase(const AsyncSqlDatabase &db, const QString &name)
{
    CHECK_QCOREAPPLICATION()
    auto *registry = s_registry();
    QWriteLocker locker(&registry->lock);

    if (registry->connections.contains(name)) {
        invalidateDb(registry->connections.take(name), name);
        qCWarning(AsyncSqlLog, "AsyncSqlDatabasePrivate::addDatabase: duplicate connection name '%ls', old "
                 "connection removed.", qUtf16Printable(name));
    }
    registry->connections.insert(name, db);
    db.d->connName = name;
}

Task<AsyncSqlDatabase> AsyncSqlDatabasePrivate::database(const QString& name, bool open)
{
    CHECK_QCOREAPPLICATION_CORO(AsyncSqlDatabase{})
    AsyncSqlDatabase db = s_registry()->connection(name);
    if (!db.isValid()) {
        co_return db;
    }
    if (db.driver()->thread() != QThread::currentThread()) {
        qCWarning(AsyncSqlLog, "AsyncSqlDatabasePrivate::database: requested database does not belong to the calling thread.");
        co_return AsyncSqlDatabase();
    }

    if (open && !db.isOpen()) {
        if (!co_await db.open()) {
            qCWarning(AsyncSqlLog) << "AsyncSqlDatabasePrivate::database: unable to open database:" << db.lastError().text();
        }
    }
    co_return db;
}


void AsyncSqlDatabasePrivate::disable()
{
    if (driver != shared_null()->driver) {
        delete driver;
        driver = shared_null()->driver;
    }
}

AsyncSqlDatabase AsyncSqlDatabase::addDatabase(const QString &type, const QString &connectionName)
{
    AsyncSqlDatabase db(type);
    AsyncSqlDatabasePrivate::addDatabase(db, connectionName);
    return db;
}

Task<AsyncSqlDatabase> AsyncSqlDatabase::database(const QString& connectionName, bool open)
{
    return AsyncSqlDatabasePrivate::database(connectionName, open);
}

void AsyncSqlDatabase::removeDatabase(const QString& connectionName)
{
    AsyncSqlDatabasePrivate::removeDatabase(connectionName);
}

QStringList AsyncSqlDatabase::drivers()
{
    CHECK_QCOREAPPLICATION(QStringList{})
    QStringList list;

    if (auto *pluginLoader  = loader(); pluginLoader) {
        for (const QString &val : pluginLoader->availableDrivers()) {
            if (!list.contains(val)) {
                list.emplace_back(val);
            }
        }
    }

    auto *registry = s_registry();
    QReadLocker locker(&registry->lock);
    const auto &dict = registry->registeredDrivers;
    for (const auto &[k, _] : dict.asKeyValueRange()) {
        if (!list.contains(k)) {
            list.emplace_back(k);
        }
    }

    return list;
}

bool AsyncSqlDatabase::contains(const QString& connectionName)
{
    CHECK_QCOREAPPLICATION(false)
    return s_registry()->connectionExists(connectionName);
}

QStringList AsyncSqlDatabase::connectionNames()
{
    CHECK_QCOREAPPLICATION(QStringList{})
    return s_registry()->connectionNames();
}

AsyncSqlDatabase::AsyncSqlDatabase(const QString &type)
   : d(new AsyncSqlDatabasePrivate(nullptr))
{
    d->init(type);
}

AsyncSqlDatabase::AsyncSqlDatabase(AsyncSqlDriver *driver)
    : d(new AsyncSqlDatabasePrivate(driver))
{
}

AsyncSqlDatabase::AsyncSqlDatabase()
    : d(AsyncSqlDatabasePrivate::shared_null())
{
    d->ref.ref();
}

AsyncSqlDatabase::AsyncSqlDatabase(const AsyncSqlDatabase &other)
{
    d = other.d;
    d->ref.ref();
}

AsyncSqlDatabase &AsyncSqlDatabase::operator=(const AsyncSqlDatabase &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

void AsyncSqlDatabasePrivate::init(const QString &type)
{
    CHECK_QCOREAPPLICATION()
    drvName = type;

    if (!driver) {
        auto *registry = s_registry();
        const QReadLocker locker(&registry->lock);
        const auto &dict = registry->registeredDrivers;
        if (const auto it = dict.find(type); it != dict.end()) {
            driver = it.value()->createDriver();
        }
    }

    if (!driver && loader()) {
        driver = loader()->loadDriver(type);
    }

    if (!driver) {
        qCWarning(AsyncSqlLog, "AsyncSqlDatabase: %ls driver not loaded", qUtf16Printable(type));
        qCWarning(AsyncSqlLog, "AsyncSqlDatabase: available drivers: %ls",
                  qUtf16Printable(AsyncSqlDatabase::drivers().join(u' ')));
        if (QCoreApplication::instance() == nullptr)
            qCWarning(AsyncSqlLog, "AsyncSqlDatabase: an instance of QCoreApplication is required for loading driver plugins");
        driver = shared_null()->driver;
    }
}

AsyncSqlDatabase::~AsyncSqlDatabase()
{
    if (!d->ref.deref()) {
        // FIXME: Can we close sync?
        QCoro::waitFor([this]() -> Task<> { co_await close(); }());
        delete d;
    }
}

Task<bool> AsyncSqlDatabase::open()
{
    return d->driver->open(d->dbname, d->userName, d->password, d->hostname,
                            d->port, d->connOptions);
}

Task<bool> AsyncSqlDatabase::open(const QString& user, const QString& password)
{
    setUserName(user);
    return d->driver->open(d->dbname, user, password, d->hostname,
                            d->port, d->connOptions);
}

Task<void> AsyncSqlDatabase::close()
{
    return d->driver->close();
}

bool AsyncSqlDatabase::isOpen() const
{
    return d->driver->isOpen();
}

bool AsyncSqlDatabase::isOpenError() const
{
    return d->driver->isOpenError();
}

Task<bool> AsyncSqlDatabase::transaction()
{
    if (!d->driver->hasFeature(QSqlDriver::Transactions)) {
        co_return false;
    }
    co_return co_await d->driver->beginTransaction();
}

Task<bool> AsyncSqlDatabase::commit()
{
    if (!d->driver->hasFeature(QSqlDriver::Transactions)) {
        co_return false;
    }
    co_return co_await d->driver->commitTransaction();
}

Task<bool> AsyncSqlDatabase::rollback()
{
    if (!d->driver->hasFeature(QSqlDriver::Transactions)) {
        co_return false;
    }
    co_return co_await d->driver->rollbackTransaction();
}

void AsyncSqlDatabase::setDatabaseName(const QString& name)
{
    if (isValid()) {
        d->dbname = name;
    }
}

void AsyncSqlDatabase::setUserName(const QString& name)
{
    if (isValid()) {
        d->userName = name;
    }
}

void AsyncSqlDatabase::setPassword(const QString& password)
{
    if (isValid()) {
        d->password = password;
    }
}

void AsyncSqlDatabase::setHostName(const QString& host)
{
    if (isValid()) {
        d->hostname = host;
    }
}

void AsyncSqlDatabase::setPort(int port)
{
    if (isValid()) {
        d->port = port;
    }
}

QString AsyncSqlDatabase::databaseName() const
{
    return d->dbname;
}

QString AsyncSqlDatabase::userName() const
{
    return d->userName;
}

QString AsyncSqlDatabase::password() const
{
    return d->password;
}

QString AsyncSqlDatabase::hostName() const
{
    return d->hostname;
}

QString AsyncSqlDatabase::driverName() const
{
    return d->drvName;
}

int AsyncSqlDatabase::port() const
{
    return d->port;
}

AsyncSqlDriver* AsyncSqlDatabase::driver() const
{
    return d->driver;
}

QSqlError AsyncSqlDatabase::lastError() const
{
    return d->driver->lastError();
}

Task<QStringList> AsyncSqlDatabase::tables(QSql::TableType type) const
{
    return d->driver->tables(type);
}

Task<QSqlIndex> AsyncSqlDatabase::primaryIndex(const QString& tablename) const
{
    return d->driver->primaryIndex(tablename);
}


Task<QSqlRecord> AsyncSqlDatabase::record(const QString& tablename) const
{
    return d->driver->record(tablename);
}

void AsyncSqlDatabase::setConnectOptions(const QString &options)
{
    if (isValid()) {
        d->connOptions = options;
    }
}

QString AsyncSqlDatabase::connectOptions() const
{
    return d->connOptions;
}

bool AsyncSqlDatabase::isDriverAvailable(const QString& name)
{
    return drivers().contains(name);
}

AsyncSqlDatabase AsyncSqlDatabase::addDatabase(AsyncSqlDriver* driver, const QString& connectionName)
{
    AsyncSqlDatabase db(driver);
    AsyncSqlDatabasePrivate::addDatabase(db, connectionName);
    return db;
}

bool AsyncSqlDatabase::isValid() const
{
    return d->driver && d->driver != d->shared_null()->driver;
}

AsyncSqlDatabase AsyncSqlDatabase::cloneDatabase(const AsyncSqlDatabase &other, const QString &connectionName)
{
    if (!other.isValid()) {
        return AsyncSqlDatabase();
    }

    AsyncSqlDatabase db(other.driverName());
    db.d->copy(other.d);
    AsyncSqlDatabasePrivate::addDatabase(db, connectionName);
    return db;
}

AsyncSqlDatabase AsyncSqlDatabase::cloneDatabase(const QString &other, const QString &connectionName)
{
    CHECK_QCOREAPPLICATION(AsyncSqlDatabase{})
    return cloneDatabase(s_registry()->connection(other), connectionName);
}

QString AsyncSqlDatabase::connectionName() const
{
    return d->connName;
}

void AsyncSqlDatabase::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
    if (driver()) {
        driver()->setNumericalPrecisionPolicy(precisionPolicy);
    }
    d->precisionPolicy = precisionPolicy;
}

QSql::NumericalPrecisionPolicy AsyncSqlDatabase::numericalPrecisionPolicy() const
{
    if (driver()) {
        return driver()->numericalPrecisionPolicy();
    } else {
        return d->precisionPolicy;
    }
}

#include "moc_qcoroasyncsqldatabase.cpp"

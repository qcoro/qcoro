#pragma once

#include "qcoroasyncsql_export.h"
#include "qcoroasyncsqldatabase.h"
#include "qcorotask.h"

#include <qtsqlglobal.h>
#include <QString>
#include <QVariant>

// clazy:excludeall=qproperty-without-notify

class QSqlError;
class QSqlRecord;

namespace QCoro
{

class AsyncSqlDriver;
class AsyncSqlResult;
class AsyncSqlQueryPrivate;

class QCOROASYNCSQL_EXPORT AsyncSqlQuery
{
    Q_GADGET
    Q_DISABLE_COPY(AsyncSqlQuery);

public:
    Q_PROPERTY(bool forwardOnly READ isForwardOnly WRITE setForwardOnly)
    Q_PROPERTY(bool positionalBindingEnabled READ isPositionalBindingEnabled WRITE setPositionalBindingEnabled)
    Q_PROPERTY(QSql::NumericalPrecisionPolicy numericalPrecisionPolicy READ numericalPrecisionPolicy WRITE setNumericalPrecisionPolicy)

    explicit AsyncSqlQuery(AsyncSqlResult *r);
    explicit AsyncSqlQuery(const QString& query = QString(), const AsyncSqlDatabase &db = AsyncSqlDatabase());
    explicit AsyncSqlQuery(const AsyncSqlDatabase &db);

    AsyncSqlQuery(AsyncSqlQuery &&other) noexcept;
    AsyncSqlQuery& operator=(AsyncSqlQuery &&other) noexcept;

    ~AsyncSqlQuery();

    void swap(AsyncSqlQuery &other) noexcept { std::swap(d, other.d); }

    bool isValid() const;
    bool isActive() const;
    bool isNull(int field) const;
    Task<bool> isNull(const QString &name) const;
    int at() const;
    QString lastQuery() const;
    int numRowsAffected() const;
    QSqlError lastError() const;
    bool isSelect() const;
    int size() const;
    const AsyncSqlDriver* driver() const;
    const AsyncSqlResult* result() const;
    bool isForwardOnly() const;
    Task<QSqlRecord> record() const;

    void setForwardOnly(bool forward);
    Task<bool> exec(const QString& query);
    QVariant value(int i) const;
    Task<QVariant> value(const QString &name) const;

    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
    QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;

    void setPositionalBindingEnabled(bool enable);
    bool isPositionalBindingEnabled() const;

    Task<bool> seek(int i, bool relative = false);
    Task<bool> next();
    Task<bool> previous();
    Task<bool> first();
    Task<bool> last();

    void clear();

    // prepared query support
    Task<bool> exec();
    enum BatchExecutionMode { ValuesAsRows, ValuesAsColumns };
    Task<bool> execBatch(BatchExecutionMode mode = ValuesAsRows);
    Task<bool> prepare(const QString& query);
    void bindValue(const QString& placeholder, const QVariant& val,
                   QSql::ParamType type = QSql::In);
    void bindValue(int pos, const QVariant& val, QSql::ParamType type = QSql::In);
    void addBindValue(const QVariant& val, QSql::ParamType type = QSql::In);
    QVariant boundValue(const QString& placeholder) const;
    QVariant boundValue(int pos) const;
    QVariantList boundValues() const;
    QStringList boundValueNames() const;
    QString boundValueName(int pos) const;
    QString executedQuery() const;
    Task<QVariant> lastInsertId() const;
    void finish();
    Task<bool> nextResult();

private:
    AsyncSqlQueryPrivate *d;
};

} // namespace QCoro
#pragma once

#include "qcoroasyncsql_export.h"
#include "qcorotask.h"

#include <qtsqlglobal.h>

class QSqlError;
class QSqlRecord;
class QVariant;

namespace QCoro
{

class AsyncSqlDriver;
class AsyncSqlResultPrivate;
class AsyncSqlQuery;
class QCOROASYNCSQL_EXPORT AsyncSqlResult
{
    friend class QCoro::AsyncSqlQuery;

public:
    virtual ~AsyncSqlResult();
    virtual QVariant handle() const;

protected:
    enum BindingSyntax {
        PositionalBinding,
        NamedBinding
    };

    explicit AsyncSqlResult(const AsyncSqlDriver * db);
    AsyncSqlResult(AsyncSqlResultPrivate &dd);
    int at() const;
    QString lastQuery() const;
    QSqlError lastError() const;
    bool isValid() const;
    bool isActive() const;
    bool isSelect() const;
    bool isForwardOnly() const;
    const AsyncSqlDriver* driver() const;

    virtual void setAt(int at);
    virtual void setActive(bool a);
    virtual void setLastError(const QSqlError& e);
    virtual void setQuery(const QString& query);
    virtual void setSelect(bool s);
    virtual void setForwardOnly(bool forward);

    // prepared query support
    virtual Task<bool> exec();
    virtual Task<bool> prepare(const QString& query);
    virtual Task<bool> safePrepare(const QString& sqlquery);
    virtual void bindValue(int pos, const QVariant& val, QSql::ParamType type);
    virtual void bindValue(const QString& placeholder, const QVariant& val,
                           QSql::ParamType type);
    void addBindValue(const QVariant& val, QSql::ParamType type);
    QVariant boundValue(const QString& placeholder) const;
    QVariant boundValue(int pos) const;
    QSql::ParamType bindValueType(const QString& placeholder) const;
    QSql::ParamType bindValueType(int pos) const;
    int boundValueCount() const;
    QVariantList &boundValues();
    QVariantList boundValues() const;

    QString executedQuery() const;
    QStringList boundValueNames() const;
    QString boundValueName(int pos) const;
    void clear();
    bool hasOutValues() const;

    BindingSyntax bindingSyntax() const;

    virtual QVariant data(int i) = 0;
    virtual bool isNull(int i) = 0;
    virtual Task<bool> reset(const QString& sqlquery) = 0;
    virtual Task<bool> fetch(int i) = 0;
    virtual Task<bool> fetchNext();
    virtual Task<bool> fetchPrevious();
    virtual Task<bool> fetchFirst() = 0;
    virtual Task<bool> fetchLast() = 0;
    virtual int size() = 0;
    virtual int numRowsAffected() = 0;
    virtual Task<QSqlRecord> record() const;
    virtual Task<QVariant> lastInsertId() const;

    enum VirtualHookOperation { };
    virtual void virtual_hook(int id, void *data);
    virtual Task<bool> execBatch(bool arrayBind = false);
    virtual void detachFromResultSet();
    virtual void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy);
    QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;
    void setPositionalBindingEnabled(bool enable);
    bool isPositionalBindingEnabled() const;
    virtual Task<bool> nextResult();
    void resetBindCount(); // HACK

    std::unique_ptr<AsyncSqlResultPrivate> d_ptr;

private:
    Q_DISABLE_COPY(AsyncSqlResult)
    Q_DECLARE_PRIVATE(AsyncSqlResult)
};

} // namespace QCoro
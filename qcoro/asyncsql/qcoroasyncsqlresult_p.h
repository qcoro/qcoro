#pragma once

#include "qcoroasyncsqlresult.h"
#include "qcoroasyncsqldriver.h"

#include <QPointer>
#include <QHash>

#include <QSqlError>

struct Placeholders {
    Placeholders(const QString &hldr = QString(), qsizetype index = -1): holderName(hldr), holderPos(index) { }
    bool operator==(const Placeholders &h) const { return h.holderPos == holderPos && h.holderName == holderName; }
    bool operator!=(const Placeholders &h) const { return h.holderPos != holderPos || h.holderName != holderName; }
    QString holderName;
    qsizetype holderPos;
};

namespace QCoro
{

class Q_SQL_EXPORT AsyncSqlResultPrivate
{
public:
    AsyncSqlResultPrivate(AsyncSqlResult *q, const AsyncSqlDriver *drv)
      : q_ptr(q),
        sqldriver(const_cast<AsyncSqlDriver *>(drv))
    { }
    virtual ~AsyncSqlResultPrivate() = default;

    void clearValues()
    {
        values.clear();
        bindCount = 0;
    }

    void resetBindCount()
    {
        bindCount = 0;
    }

    void clearIndex()
    {
        indexes.clear();
        holders.clear();
        types.clear();
    }

    void clear()
    {
        clearValues();
        clearIndex();
    }

    inline AsyncSqlDriverPrivate *drv_d_func() {
        return sqldriver ? sqldriver->d_func() : nullptr;
    }
    inline const AsyncSqlDriverPrivate *drv_d_func() const {
        return sqldriver ? sqldriver->d_func() : nullptr;
    }

    virtual QString fieldSerial(qsizetype) const;
    QString positionalToNamedBinding(const QString &query) const;
    QString namedToPositionalBinding(const QString &query);
    QString holderAt(int index) const;

    AsyncSqlResult *q_ptr = nullptr;
    QPointer<AsyncSqlDriver> sqldriver;

    QString sql;
    QSqlError error;

    QString executedQuery;
    QHash<int, QSql::ParamType> types;
    QList<QVariant> values;
    using IndexMap = QHash<QString, QList<int>>;
    IndexMap indexes;

    using PlaceHolderList = QList<Placeholders>;
    PlaceHolderList holders;

    AsyncSqlResult::BindingSyntax binds = AsyncSqlResult::PositionalBinding;
    QSql::NumericalPrecisionPolicy precisionPolicy = QSql::LowPrecisionDouble;
    int idx = QSql::BeforeFirstRow;
    int bindCount = 0;
    bool active = false;
    bool isSelect = false;
    bool forwardOnly = false;
    bool positionalBindingEnabled = true;

    static bool isVariantNull(const QVariant &variant);

protected:
    Q_DECLARE_PUBLIC(AsyncSqlResult)
};

} // namespace QCoro
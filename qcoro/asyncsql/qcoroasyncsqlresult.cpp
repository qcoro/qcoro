#include "qcoroasyncsqlresult.h"
#include "qcoroasyncsqldriver.h"
#include "qcoroasyncsqldriver_p.h"
#include "qcoroasyncsqlresult_p.h"

#include <QList>
#include <QUuid>
#include <QDateTime>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>

using namespace Qt::StringLiterals;
using namespace QCoro;

QString AsyncSqlResultPrivate::holderAt(int index) const
{
    return holders.size() > index ? holders.at(index).holderName : fieldSerial(index);
}

QString AsyncSqlResultPrivate::fieldSerial(qsizetype i) const
{
    return QString(":%1"_L1).arg(i);
}

static bool qIsAlnum(QChar ch)
{
    uint u = uint(ch.unicode());
    // matches [a-zA-Z0-9_]
    return u - 'a' < 26 || u - 'A' < 26 || u - '0' < 10 || u == '_';
}

QString AsyncSqlResultPrivate::positionalToNamedBinding(const QString &query) const
{
    if (!positionalBindingEnabled)
        return query;

    const qsizetype n = query.size();

    QString result;
    result.reserve(n * 5 / 4);
    QChar closingQuote;
    qsizetype count = 0;
    bool ignoreBraces = (sqldriver->dbmsType() == AsyncSqlDriver::PostgreSQL);

    for (qsizetype i = 0; i < n; ++i) {
        QChar ch = query.at(i);
        if (!closingQuote.isNull()) {
            if (ch == closingQuote) {
                if (closingQuote == u']'
                    && i + 1 < n && query.at(i + 1) == closingQuote) {
                    // consume the extra character. don't close.
                    ++i;
                    result += ch;
                } else {
                    closingQuote = QChar();
                }
            }
            result += ch;
        } else {
            if (ch == u'?') {
                result += fieldSerial(count++);
            } else {
                if (ch == u'\'' || ch == u'"' || ch == u'`')
                    closingQuote = ch;
                else if (!ignoreBraces && ch == u'[')
                    closingQuote = u']';
                result += ch;
            }
        }
    }
    result.squeeze();
    return result;
}

QString AsyncSqlResultPrivate::namedToPositionalBinding(const QString &query)
{
    const qsizetype n = query.size();

    QString result;
    result.reserve(n);
    QChar closingQuote;
    int count = 0;
    qsizetype i = 0;
    bool ignoreBraces = (sqldriver->dbmsType() == AsyncSqlDriver::PostgreSQL);
    const bool qmarkNotationSupported = (sqldriver->dbmsType() != AsyncSqlDriver::PostgreSQL);

    while (i < n) {
        QChar ch = query.at(i);
        if (!closingQuote.isNull()) {
            if (ch == closingQuote) {
                if (closingQuote == u']'
                        && i + 1 < n && query.at(i + 1) == closingQuote) {
                    // consume the extra character. don't close.
                    ++i;
                    result += ch;
                } else {
                    closingQuote = QChar();
                }
            }
            result += ch;
            ++i;
        } else {
            if (ch == u':'
                    && (i == 0 || query.at(i - 1) != u':')
                    && (i + 1 < n && qIsAlnum(query.at(i + 1)))) {
                int pos = i + 2;
                while (pos < n && qIsAlnum(query.at(pos)))
                    ++pos;
                // if question mark notation is not supported we have to use
                // the native binding.  fieldSerial() should be renamed
                // to toNativeBinding() and used unconditionally here
                if (qmarkNotationSupported)
                    result += u'?';
                else
                    result += fieldSerial(count);
                QString holder(query.mid(i, pos - i));
                indexes[holder].append(count++);
                holders.append(Placeholders(holder, i));
                i = pos;
            } else {
                if (ch == u'\'' || ch == u'"' || ch == u'`')
                    closingQuote = ch;
                else if (!ignoreBraces && ch == u'[')
                    closingQuote = u']';
                result += ch;
                ++i;
            }
        }
    }
    result.squeeze();
    values.resize(holders.size());
    return result;
}


AsyncSqlResult::AsyncSqlResult(const AsyncSqlDriver *db)
{
    d_ptr = std::make_unique<AsyncSqlResultPrivate>(this, db);
    Q_D(AsyncSqlResult);
    if (d->sqldriver) {
        setNumericalPrecisionPolicy(d->sqldriver->numericalPrecisionPolicy());
    }
}

AsyncSqlResult::AsyncSqlResult(AsyncSqlResultPrivate &dd)
    : d_ptr(&dd)
{
    Q_D(AsyncSqlResult);
    if (d->sqldriver) {
        setNumericalPrecisionPolicy(d->sqldriver->numericalPrecisionPolicy());
    }
}

AsyncSqlResult::~AsyncSqlResult() = default;


void AsyncSqlResult::setQuery(const QString& query)
{
    Q_D(AsyncSqlResult);
    d->sql = query;
}

QString AsyncSqlResult::lastQuery() const
{
    Q_D(const AsyncSqlResult);
    return d->sql;
}

int AsyncSqlResult::at() const
{
    Q_D(const AsyncSqlResult);
    return d->idx;
}

bool AsyncSqlResult::isValid() const
{
    Q_D(const AsyncSqlResult);
    return d->idx != QSql::BeforeFirstRow && d->idx != QSql::AfterLastRow;
}

bool AsyncSqlResult::isActive() const
{
    Q_D(const AsyncSqlResult);
    return d->active;
}

void AsyncSqlResult::setAt(int index)
{
    Q_D(AsyncSqlResult);
    d->idx = index;
}

void AsyncSqlResult::setSelect(bool select)
{
    Q_D(AsyncSqlResult);
    d->isSelect = select;
}

bool AsyncSqlResult::isSelect() const
{
    Q_D(const AsyncSqlResult);
    return d->isSelect;
}

const AsyncSqlDriver *AsyncSqlResult::driver() const
{
    Q_D(const AsyncSqlResult);
    return d->sqldriver;
}

void AsyncSqlResult::setActive(bool active)
{
    Q_D(AsyncSqlResult);
    if (active) {
        d->executedQuery = d->sql;
    }

    d->active = active;
}

void AsyncSqlResult::setLastError(const QSqlError &error)
{
    Q_D(AsyncSqlResult);
    d->error = error;
}

QSqlError AsyncSqlResult::lastError() const
{
    Q_D(const AsyncSqlResult);
    return d->error;
}

Task<bool> AsyncSqlResult::fetchNext()
{
    return fetch(at() + 1);
}

Task<bool> AsyncSqlResult::fetchPrevious()
{
    return fetch(at() - 1);
}

bool AsyncSqlResult::isForwardOnly() const
{
    Q_D(const AsyncSqlResult);
    return d->forwardOnly;
}

void AsyncSqlResult::setForwardOnly(bool forward)
{
    Q_D(AsyncSqlResult);
    d->forwardOnly = forward;
}

Task<bool> AsyncSqlResult::safePrepare(const QString& query)
{
    Q_D(AsyncSqlResult);
    if (!driver()) {
        co_return false;
    }
    d->clear();
    d->sql = query;
    if (!driver()->hasFeature(QSqlDriver::PreparedQueries)) {
        co_return co_await prepare(query);
    }

    // parse the query to memorize parameter location
    d->executedQuery = d->namedToPositionalBinding(query);

    if (driver()->hasFeature(QSqlDriver::NamedPlaceholders)) {
        d->executedQuery = d->positionalToNamedBinding(query);
    }

    co_return co_await prepare(d->executedQuery);
}

Task<bool> AsyncSqlResult::prepare(const QString& query)
{
    Q_D(AsyncSqlResult);
    d->sql = query;
    if (d->holders.isEmpty()) {
        // parse the query to memorize parameter location
        d->namedToPositionalBinding(query);
    }
    co_return true; // fake prepares should always succeed
}

bool AsyncSqlResultPrivate::isVariantNull(const QVariant &variant)
{
    if (variant.isNull()) {
        return true;
    }

    switch (variant.typeId()) {
    case qMetaTypeId<QString>():
        return static_cast<const QString*>(variant.constData())->isNull();
    case qMetaTypeId<QByteArray>():
        return static_cast<const QByteArray*>(variant.constData())->isNull();
    case qMetaTypeId<QDateTime>():
        // We treat invalid date-time as null, since its ISODate would be empty.
        return !static_cast<const QDateTime*>(variant.constData())->isValid();
    case qMetaTypeId<QDate>():
        return static_cast<const QDate*>(variant.constData())->isNull();
    case qMetaTypeId<QTime>():
        // As for QDateTime, QTime can be invalid without being null.
        return !static_cast<const QTime*>(variant.constData())->isValid();
    case qMetaTypeId<QUuid>():
        return static_cast<const QUuid*>(variant.constData())->isNull();
    default:
        break;
    }

    return false;
}

Task<bool> AsyncSqlResult::exec()
{
    Q_D(AsyncSqlResult);
    bool ret;
    // fake preparation - just replace the placeholders..
    QString query = lastQuery();
    if (d->binds == NamedBinding) {
        for (qsizetype i = d->holders.size() - 1; i >= 0; --i) {
            const QString &holder = d->holders.at(i).holderName;
            const QVariant val = d->values.value(d->indexes.value(holder).value(0,-1));
            QSqlField f(""_L1, val.metaType());
            if (AsyncSqlResultPrivate::isVariantNull(val)) {
                f.setValue(QVariant());
            } else {
                f.setValue(val);
            }
            query = query.replace(d->holders.at(i).holderPos,
                                   holder.size(), driver()->formatValue(f));
        }
    } else {
        qsizetype i = 0;
        for (const QVariant &var : std::as_const(d->values)) {
            i = query.indexOf(u'?', i);
            if (i == -1) {
                continue;
            }
            QSqlField f(""_L1, var.metaType());
            if (AsyncSqlResultPrivate::isVariantNull(var)) {
                f.clear();
            } else {
                f.setValue(var);
            }
            const QString val = driver()->formatValue(f);
            query = query.replace(i, 1, val);
            i += val.size();
        }
    }

    // have to retain the original query with placeholders
    QString orig = lastQuery();
    ret = co_await reset(query);
    d->executedQuery = query;
    setQuery(orig);
    d->resetBindCount();
    co_return ret;
}

void AsyncSqlResult::bindValue(int index, const QVariant& val, QSql::ParamType paramType)
{
    Q_D(AsyncSqlResult);
    d->binds = PositionalBinding;
    QList<int> &indexes = d->indexes[d->fieldSerial(index)];
    if (!indexes.contains(index)) {
        indexes.append(index);
    }
    if (d->values.size() <= index) {
        d->values.resize(index + 1);
    }
    d->values[index] = val;
    if (paramType != QSql::In || !d->types.isEmpty()) {
        d->types[index] = paramType;
    }
}

void AsyncSqlResult::bindValue(const QString& placeholder, const QVariant& val,
                           QSql::ParamType paramType)
{
    Q_D(AsyncSqlResult);
    d->binds = NamedBinding;
    // if the index has already been set when doing emulated named
    // bindings - don't reset it
    const QList<int> indexes = d->indexes.value(placeholder);
    for (int idx : indexes) {
        if (d->values.size() <= idx) {
            d->values.resize(idx + 1);
        }
        d->values[idx] = val;
        if (paramType != QSql::In || !d->types.isEmpty()) {
            d->types[idx] = paramType;
        }
    }
}

void AsyncSqlResult::addBindValue(const QVariant& val, QSql::ParamType paramType)
{
    Q_D(AsyncSqlResult);
    d->binds = PositionalBinding;
    bindValue(d->bindCount, val, paramType);
    ++d->bindCount;
}

QVariant AsyncSqlResult::boundValue(int index) const
{
    Q_D(const AsyncSqlResult);
    return d->values.value(index);
}

QVariant AsyncSqlResult::boundValue(const QString& placeholder) const
{
    Q_D(const AsyncSqlResult);
    const QList<int> indexes = d->indexes.value(placeholder);
    return d->values.value(indexes.value(0,-1));
}

QSql::ParamType AsyncSqlResult::bindValueType(int index) const
{
    Q_D(const AsyncSqlResult);
    return d->types.value(index, QSql::In);
}

QSql::ParamType AsyncSqlResult::bindValueType(const QString& placeholder) const
{
    Q_D(const AsyncSqlResult);
    return d->types.value(d->indexes.value(placeholder).value(0,-1), QSql::In);
}

int AsyncSqlResult::boundValueCount() const
{
    Q_D(const AsyncSqlResult);
    return d->values.size();
}

QVariantList AsyncSqlResult::boundValues() const
{
    Q_D(const AsyncSqlResult);
    return d->values;
}

QVariantList &AsyncSqlResult::boundValues()
{
    Q_D(AsyncSqlResult);
    return d->values;
}


AsyncSqlResult::BindingSyntax AsyncSqlResult::bindingSyntax() const
{
    Q_D(const AsyncSqlResult);
    return d->binds;
}

void AsyncSqlResult::clear()
{
    Q_D(AsyncSqlResult);
    d->clear();
}

QString AsyncSqlResult::executedQuery() const
{
    Q_D(const AsyncSqlResult);
    return d->executedQuery;
}

void AsyncSqlResult::resetBindCount()
{
    Q_D(AsyncSqlResult);
    d->resetBindCount();
}

QStringList AsyncSqlResult::boundValueNames() const
{
    Q_D(const AsyncSqlResult);
    QList<QString> ret;
    ret.reserve(d->holders.size());
    for (const Placeholders &holder : std::as_const(d->holders)) {
        ret.push_back(holder.holderName);
    }
    return ret;
}

QString AsyncSqlResult::boundValueName(int index) const
{
    Q_D(const AsyncSqlResult);
    return d->holderAt(index);
}

bool AsyncSqlResult::hasOutValues() const
{
    Q_D(const AsyncSqlResult);
    if (d->types.isEmpty()) {
        return false;
    }

    return std::any_of(d->types.begin(), d->types.end(), [](const auto &type) {
        return type != QSql::In;
    });
}

Task<QSqlRecord> AsyncSqlResult::record() const
{
    co_return QSqlRecord();
}

Task<QVariant> AsyncSqlResult::lastInsertId() const
{
    co_return QVariant();
}

void AsyncSqlResult::virtual_hook(int, void *)
{
}

Task<bool> AsyncSqlResult::execBatch(bool arrayBind)
{
    Q_UNUSED(arrayBind);
    Q_D(AsyncSqlResult);

    const auto values = d->values;
    if (values.empty()) {
        co_return false;
    }
    const qsizetype batchCount = values.at(0).toList().size();
    const qsizetype valueCount = values.size();
    for (qsizetype i = 0; i < batchCount; ++i) {
        for (qsizetype j = 0; j < valueCount; ++j) {
            bindValue(j, values.at(j).toList().at(i), QSql::In);
        }
        if (!co_await exec()) {
            co_return false;
        }
    }
    co_return true;
}

void AsyncSqlResult::detachFromResultSet()
{
}

void AsyncSqlResult::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy)
{
    Q_D(AsyncSqlResult);
    d->precisionPolicy = policy;
}

QSql::NumericalPrecisionPolicy AsyncSqlResult::numericalPrecisionPolicy() const
{
    Q_D(const AsyncSqlResult);
    return d->precisionPolicy;
}

void AsyncSqlResult::setPositionalBindingEnabled(bool enable)
{
    Q_D(AsyncSqlResult);
    d->positionalBindingEnabled = enable;
}

bool AsyncSqlResult::isPositionalBindingEnabled() const
{
    Q_D(const AsyncSqlResult);
    return d->positionalBindingEnabled;
}

Task<bool> AsyncSqlResult::nextResult()
{
    co_return false;
}

QVariant AsyncSqlResult::handle() const
{
    return QVariant();
}

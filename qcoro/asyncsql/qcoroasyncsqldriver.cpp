#include "qcoroasyncsqldriver.h"
#include "qcoroasyncsqldriver_p.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>

#include <limits.h>

using namespace QCoro;
using namespace Qt::StringLiterals;

static QString prepareIdentifier(const QString &identifier,
        AsyncSqlDriver::IdentifierType type, const AsyncSqlDriver *driver)
{
    Q_ASSERT(driver != nullptr);
    QString ret = identifier;
    if (!driver->isIdentifierEscaped(identifier, type))
        ret = driver->escapeIdentifier(identifier, type);
    return ret;
}

AsyncSqlDriver::AsyncSqlDriver(QObject *parent)
    : QObject(parent)
    , d_ptr(new AsyncSqlDriverPrivate)
{
}

AsyncSqlDriver::AsyncSqlDriver(AsyncSqlDriverPrivate *dd, QObject *parent)
    : QObject(parent)
    , d_ptr(dd)
{
}

AsyncSqlDriver::~AsyncSqlDriver() = default;

bool AsyncSqlDriver::isOpen() const
{
    Q_D(const AsyncSqlDriver);
    return d->isOpen;
}

bool AsyncSqlDriver::isOpenError() const
{
    Q_D(const AsyncSqlDriver);
    return d->isOpenError;
}

void AsyncSqlDriver::setOpen(bool open)
{
    Q_D(AsyncSqlDriver);
    d->isOpen = open;
}


void AsyncSqlDriver::setOpenError(bool error)
{
    Q_D(AsyncSqlDriver);
    d->isOpenError = error;
    if (error) {
        d->isOpen = false;
    }
}

Task<bool> AsyncSqlDriver::beginTransaction()
{
    co_return false;
}

Task<bool> AsyncSqlDriver::commitTransaction()
{
    co_return false;
}

Task<bool> AsyncSqlDriver::rollbackTransaction()
{
    co_return false;
}

void AsyncSqlDriver::setLastError(const QSqlError &error)
{
    Q_D(AsyncSqlDriver);
    d->error = error;
}

QSqlError AsyncSqlDriver::lastError() const
{
    Q_D(const AsyncSqlDriver);
    return d->error;
}

Task<QStringList> AsyncSqlDriver::tables(QSql::TableType) const
{
    co_return QStringList();
}

Task<QSqlIndex> AsyncSqlDriver::primaryIndex(const QString&) const
{
    co_return QSqlIndex();
}

Task<QSqlRecord> AsyncSqlDriver::record(const QString & /* tableName */) const
{
    co_return QSqlRecord();
}

QString AsyncSqlDriver::escapeIdentifier(const QString &identifier, IdentifierType) const
{
    return identifier;
}

bool AsyncSqlDriver::isIdentifierEscaped(QStringView identifier, IdentifierType type) const
{
    Q_UNUSED(type);
    return identifier.size() > 2
        && identifier.startsWith(u'"') //left delimited
        && identifier.endsWith(u'"'); //right delimited
}

QString AsyncSqlDriver::stripDelimiters(QStringView identifier, IdentifierType type) const
{
    if (isIdentifierEscaped(identifier, type)) {
        identifier = identifier.mid(1);
        identifier.chop(1);
        return identifier.toString();
    }

    return identifier.toString();
}

QString AsyncSqlDriver::sqlStatement(StatementType type, const QString &tableName,
                                 const QSqlRecord &rec, bool preparedStatement) const
{
    const auto tableNameString = tableName.isEmpty() ? QString()
                                    : prepareIdentifier(tableName, QSqlDriver::TableName, this);
    QString s;
    s.reserve(128);
    switch (type) {
    case QSqlDriver::SelectStatement:
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (rec.isGenerated(i)) {
                s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this)).append(", "_L1);
            }
        }
        if (s.isEmpty()) {
            return s;
        }
        s.chop(2);
        s = "SELECT "_L1 + s + " FROM "_L1 + tableNameString;
        break;
    case QSqlDriver::WhereStatement:
    {
        const QString tableNamePrefix = tableNameString.isEmpty()
                                            ? QString() : tableNameString + u'.';
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (!rec.isGenerated(i)) {
                continue;
            }
            s.append(s.isEmpty() ? "WHERE "_L1 : " AND "_L1);
            s.append(tableNamePrefix);
            s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this));
            if (rec.isNull(i)) {
                s.append(" IS NULL"_L1);
            } else if (preparedStatement) {
                s.append(" = ?"_L1);
            } else {
                s.append(" = "_L1).append(formatValue(rec.field(i)));
            }
        }
        break;
    }
    case QSqlDriver::UpdateStatement:
        s = s + "UPDATE "_L1 + tableNameString + " SET "_L1;
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (!rec.isGenerated(i)) {
                continue;
            }
            s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this)).append(u'=');
            if (preparedStatement) {
                s.append(u'?');
            } else {
                s.append(formatValue(rec.field(i)));
            }
            s.append(", "_L1);
        }
        if (s.endsWith(", "_L1)) {
            s.chop(2);
        } else {
            s.clear();
        }
        break;
    case QSqlDriver::DeleteStatement:
        s = s + "DELETE FROM "_L1 + tableNameString;
        break;
    case QSqlDriver::InsertStatement: {
        s = s + "INSERT INTO "_L1 + tableNameString + " ("_L1;
        QString vals;
        for (qsizetype i = 0; i < rec.count(); ++i) {
            if (!rec.isGenerated(i)) {
                continue;
            }
            s.append(prepareIdentifier(rec.fieldName(i), QSqlDriver::FieldName, this)).append(", "_L1);
            if (preparedStatement) {
                vals.append(u'?');
            } else {
                vals.append(formatValue(rec.field(i)));
            }
            vals.append(", "_L1);
        }
        if (vals.isEmpty()) {
            s.clear();
        } else {
            vals.chop(2); // remove trailing comma
            s[s.size() - 2] = u')';
            s.append("VALUES ("_L1).append(vals).append(u')');
        }
        break;
    }
    }
    return s;
}

QString AsyncSqlDriver::formatValue(const QSqlField &field, bool trimStrings) const
{
    const auto nullTxt = "NULL"_L1;

    QString r;
    if (field.isNull()) {
        r = nullTxt;
    } else {
        switch (field.metaType().id()) {
        case QMetaType::Int:
        case QMetaType::UInt:
            if (field.value().userType() == QMetaType::Bool) {
                r = field.value().toBool() ? "1"_L1 : "0"_L1;
            } else {
                r = field.value().toString();
            }
            break;
        case QMetaType::QDate:
            if (field.value().toDate().isValid()) {
                r = u'\'' + field.value().toDate().toString(Qt::ISODate) + u'\'';
            } else {
                r = nullTxt;
            }
            break;
        case QMetaType::QTime:
            if (field.value().toTime().isValid()) {
                r =  u'\'' + field.value().toTime().toString(Qt::ISODate) + u'\'';
            } else {
                r = nullTxt;
            }
            break;
        case QMetaType::QDateTime:
            if (field.value().toDateTime().isValid()) {
                r = u'\'' + field.value().toDateTime().toString(Qt::ISODate) + u'\'';
            } else {
                r = nullTxt;
            }
            break;
        case QMetaType::QString:
        case QMetaType::QChar:
        {
            QString result = field.value().toString();
            if (trimStrings) {
                int end = result.size();
                while (end && result.at(end-1).isSpace()) { /* skip white space from end */
                    end--;
                }
                result.truncate(end);
            }
            /* escape the "'" character */
            result.replace(u'\'', "''"_L1);
            r = u'\'' + result + u'\'';
            break;
        }
        case QMetaType::Bool:
            r = QString::number(field.value().toBool());
            break;
        case QMetaType::QByteArray : {
            if (hasFeature(QSqlDriver::BLOB)) {
                const QByteArray ba = field.value().toByteArray();
                r.reserve((ba.size() + 1) * 2);
                r += u'\'' + QString::fromLatin1(ba.toHex()) + u'\'';
                break;
            }
        }
            Q_FALLTHROUGH();
        default:
            r = field.value().toString();
            break;
        }
    }
    return r;
}

QVariant AsyncSqlDriver::handle() const
{
    return QVariant();
}

void AsyncSqlDriver::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
    Q_D(AsyncSqlDriver);
    d->precisionPolicy = precisionPolicy;
}

QSql::NumericalPrecisionPolicy AsyncSqlDriver::numericalPrecisionPolicy() const
{
    Q_D(const AsyncSqlDriver);
    return d->precisionPolicy;
}

AsyncSqlDriver::DbmsType AsyncSqlDriver::dbmsType() const
{
    Q_D(const AsyncSqlDriver);
    return d->dbmsType;
}

bool AsyncSqlDriver::cancelQuery()
{
    return false;
}

int AsyncSqlDriver::maximumIdentifierLength(AsyncSqlDriver::IdentifierType type) const
{
    Q_UNUSED(type);
    return INT_MAX;
}

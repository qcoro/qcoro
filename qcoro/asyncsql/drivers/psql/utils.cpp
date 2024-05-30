#include "utils.h"

#include "psql_driver_p.h"

#include <QLatin1String>

using namespace Qt::StringLiterals;

QSqlError makeError(const QString &err, QSqlError::ErrorType type, const QCoroAsyncPsqlDriverPrivate *p, PGresult *result)
{
    const char *s = PQerrorMessage(p->conn);
    QString msg = QString::fromUtf8(s);
    QString errorCode;
    if (result) {
        errorCode = QString::fromLatin1(PQresultErrorField(result, PG_DIAG_SQLSTATE));
        msg += QString::fromLatin1("(%1)").arg(errorCode);
    }
    return QSqlError("QPSQL: "_L1 + err, msg, type, errorCode);
}
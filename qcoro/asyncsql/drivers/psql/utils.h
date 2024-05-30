#pragma once
#include <QSqlError>

#include <libpq-fe.h>

class QCoroAsyncPsqlDriverPrivate;

QSqlError makeError(const QString &err, QSqlError::ErrorType type, const QCoroAsyncPsqlDriverPrivate *p, PGresult *result = nullptr);


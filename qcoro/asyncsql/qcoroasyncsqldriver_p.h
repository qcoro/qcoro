// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
// SPDX-FileCopyrightText: 2016 The Qt Company Ltd.
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroasyncsqldriver.h"
#include <QSqlError>

namespace QCoro
{

class AsyncSqlDriverPrivate
{
public:
    explicit AsyncSqlDriverPrivate(AsyncSqlDriver::DbmsType type = AsyncSqlDriver::UnknownDbms)
        : dbmsType(type)
    { }
    virtual ~AsyncSqlDriverPrivate() = default;

    QSqlError error;
    QSql::NumericalPrecisionPolicy precisionPolicy = QSql::LowPrecisionDouble;
    AsyncSqlDriver::DbmsType dbmsType;
    bool isOpen = false;
    bool isOpenError = false;

protected:
    AsyncSqlDriver *q_ptr = nullptr;

private:
    Q_DECLARE_PUBLIC(AsyncSqlDriver)
};

} // namespace QCoro

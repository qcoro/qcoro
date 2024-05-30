// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroasyncsql_export.h"

#include <QObject>
#include <QString>

#define QCoroAsyncSqlDriverPlugin_iid "cz.dvratil.qcoro.AsyncSqlDriverPlugin"

namespace QCoro
{

class AsyncSqlDriver;
class QCOROASYNCSQL_EXPORT AsyncSqlDriverPlugin : public QObject
{
    Q_OBJECT
public:
    explicit AsyncSqlDriverPlugin(QObject *parent = nullptr):
        QObject(parent) {}

    ~AsyncSqlDriverPlugin() override = default;

    virtual AsyncSqlDriver *create(const QString &key) = 0;
};

} // namespace QCoro
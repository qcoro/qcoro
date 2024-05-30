# SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

include(ECMQtDeclareLoggingCategory)

add_qcoro_library(
    NAME AsyncSql
    SOURCES
        driverpluginloader.cpp
        qcoroasyncsqldatabase.cpp
        qcoroasyncsqldriver.cpp
        qcoroasyncsqldriverplugin.cpp
        qcoroasyncsqlnulldriver.cpp
        qcoroasyncsqlquery.cpp
        qcoroasyncsqlresult.cpp
    CAMELCASE_HEADERS
        QCoroAsyncSqlDatabase
        QCoroAsyncSqlDriver
        QCoroAsyncSqlQuery
        QCoroAsyncSqlResult
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
        PUBLIC Core Sql
)

ecm_qt_declare_logging_category(${QCORO_TARGET_PREFIX}AsyncSql
    HEADER qcoroasyncsql_log.h
    IDENTIFIER AsyncSqlLog
    CATEGORY_NAME cz.dvratil.qcoro.asyncsql
    DESCRIPTION "QCoro AsyncSql"
    EXPORT QCORO
)

add_subdirectory(drivers)
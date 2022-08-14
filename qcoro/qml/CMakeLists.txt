# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
# SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME Qml
    INCLUDEDIR Qml
    SOURCES
        qcoroqmltask.cpp
        qcoroqml.cpp
    CAMELCASE_HEADERS
        QCoroQmlTask
        QCoroQml
    QT_LINK_LIBRARIES
        PUBLIC Core Qml
    QCORO_LINK_LIBRARIES
        PUBLIC Coro
)

if (${QT_VERSION_MAJOR} STREQUAL "6")
    target_link_libraries(QCoro${QT_VERSION_MAJOR}Qml PRIVATE Qt::QmlPrivate)
endif()

# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    CAMELCASE_HEADERS
        Task
        QCoro
    HEADERS
        concepts_p.h
        coroutine.h
        macros_p.h
        task.h
        waitoperationbase_p.h
    QT_LINK_LIBRARIES
        PUBLIC Core
)

############################

add_subdirectory(core)

if (QCORO_WITH_QTDBUS)
    add_subdirectory(dbus)
endif()

if (QCORO_WITH_QTNETWORK)
    add_subdirectory(network)
endif()


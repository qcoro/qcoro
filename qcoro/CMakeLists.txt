# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

set(qcoro_HEADERS
    concepts.h
    coro.h
    coroutine.h
    macros.h
    task.h
    waitoperationbase_p.h
)

add_library(QCoro INTERFACE)

target_include_directories(QCoro
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

install(
    FILES ${qcoro_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

install(TARGETS QCoro EXPORT QCoroTargets)

############################

add_subdirectory(core)

if (QCORO_WITH_QTDBUS)
    add_subdirectory(dbus)
endif()

if (QCORO_WITH_QTNETWORK)
    add_subdirectory(network)
endif()


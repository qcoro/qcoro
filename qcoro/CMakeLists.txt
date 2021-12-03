# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

set(qcoro_HEADERS
    concepts_p.h
    coroutine.h
    macros_p.h
    task.h
    waitoperationbase_p.h
)


generate_headers(
    qcoro_CamelCase_HEADERS
    HEADER_NAMES
        Task
        QCoro
    OUTPUT_DIR QCoro
    ORIGINAL_HEADERS_VAR qcoro_HEADERS
)

add_library(QCoro INTERFACE)

target_include_directories(QCoro
    INTERFACE $<BUILD_INTERFACE:${QCORO_SOURCE_DIR}>;$<BUILD_INTERFACE:${QCORO_SOURCE_DIR}/qcoro>
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>;$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qcoro>
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/QCoro>
)

install(
    FILES ${qcoro_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

install(
    FILES ${qcoro_CamelCase_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QCoro
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


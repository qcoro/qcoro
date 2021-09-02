# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

set(qcoro_core_SRCS
    qcoroiodevice.cpp
    qcoroprocess.cpp
    qcorotimer.cpp
)
add_library(QCoroCore ${qcoro_core_SRCS})
add_library(QCoro::Core ALIAS QCoroCore)
target_link_libraries(QCoroCore
    PUBLIC
    QCoro
    Qt${QT_VERSION_MAJOR}::Core
)
target_include_directories(QCoroCore
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>;$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/qcoro/core>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>;$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qcoro>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/QCoro>
)
set_target_properties(QCoroCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)

generate_headers(
    qcoro_core_CamelCase_HEADERS
    HEADER_NAMES
        QCoroCore
        QCoroIODevice
        QCoroProcess
        QCoroSignal
        QCoroTimer
    OUTPUT_DIR QCoro
    ORIGINAL_HEADERS_VAR qcoro_core_HEADERS
)

if (QCORO_QT_HAS_COMPAT_ABI)
    generate_headers(
        qcoro_core_CamelCase_HEADERS
        HEADER_NAMES
            QCoroFuture
        OUTPUT_DIR QCoro
        ORIGINAL_HEADERS_VAR qcoro_core_HEADERS
    )
endif()

install(
    FILES ${qcoro_core_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)
install(
    FILES ${qcoro_core_CamelCase_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QCoro
    COMPONENT Devel
)

install(TARGETS QCoroCore EXPORT QCoroTargets)


# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

check_qt_feature(process Core QCOROCORE_WITH_PROCESS)
check_qt_feature(future Core QCOROCORE_WITH_FUTURE)

if (QCOROCORE_WITH_PROCESS)
    set(process_SRC qcoroprocess.cpp)
    set(process_HDR QCoroProcess)
endif()
if (QCOROCORE_WITH_FUTURE)
    set(future_HDR QCoroFuture)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/qcorocore-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/qcorocore-config.h
)

add_qcoro_library(
    NAME Core
    INCLUDEDIR Core
    SOURCES
        qcoroiodevice.cpp
        qcorotimer.cpp
        ${process_SRC}
    CAMELCASE_HEADERS
        QCoroCore
        QCoroIODevice
        QCoroSignal
        QCoroTimer
        ${process_HDR}
        ${future_HDR}
    QT_LINK_LIBRARIES
        PUBLIC Core
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/qcorocore-config.h
    DESTINATION ${QCORO_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

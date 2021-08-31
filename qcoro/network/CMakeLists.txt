# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

set(qcoro_network_SRCS
    qcoroabstractsocket.cpp
    qcorolocalsocket.cpp
    qcoronetworkreply.cpp
    qcorotcpserver.cpp
)
add_library(QCoroNetwork ${qcoro_network_SRCS})
add_library(QCoro::Network ALIAS QCoroNetwork)
target_link_libraries(QCoroNetwork
    PUBLIC
    QCoroCore
    Qt${QT_VERSION_MAJOR}::Network
)
target_include_directories(QCoroNetwork
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(QCoroNetwork PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)

set(qcoro_network_HEADERS
    network.h
    qcoroabstractsocket.h
    qcorolocalsocket.h
    qcoronetworkreply.h
    qcorotcpserver.h
)

install(
    FILES ${qcoro_network_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro/network
    COMPONENT Devel
)

install(TARGETS QCoroNetwork EXPORT QCoroTargets)


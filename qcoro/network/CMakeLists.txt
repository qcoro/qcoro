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
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>;$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/qcoro/network>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qcoro>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/QCoro>
)
set_target_properties(QCoroNetwork PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)
set_target_properties(QCoroNetwork PROPERTIES VERSION ${qcoro_VERSION} SOVERSION ${qcoro_SOVERSION})

generate_headers(
    qcoro_network_CamelCase_HEADERS
    HEADER_NAMES
        QCoroNetwork
        QCoroAbstractSocket
        QCoroLocalSocket
        QCoroNetworkReply
        QCoroTcpServer
    OUTPUT_DIR QCoro/Network
    ORIGINAL_HEADERS_VAR qcoro_network_HEADERS
)

install(
    FILES ${qcoro_network_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)
install(
    FILES ${qcoro_network_CamelCase_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QCoro
    COMPONENT Devel
)

install(TARGETS QCoroNetwork EXPORT QCoroTargets)


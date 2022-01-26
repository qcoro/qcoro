# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

check_qt_feature(localserver Network QCORONETWORK_WITH_LOCALSERVER)

if (QCORONETWORK_WITH_LOCALSERVER)
    set(localserver_SRCS
        qcorolocalsocket.cpp
    )
    set(localserver_HDRS
        QCoroLocalSocket
    )
endif()

configure_file(qcoronetwork-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/qcoronetwork-config.h
)

add_qcoro_library(
    NAME Network
    SOURCES
        qcoroabstractsocket.cpp
        qcoronetworkreply.cpp
        qcorotcpserver.cpp
        ${localserver_SRCS}
    CAMELCASE_HEADERS
        QCoroNetwork
        QCoroAbstractSocket
        QCoroNetworkReply
        QCoroTcpServer
        ${localserver_HDRS}
    QCORO_LINK_LIBRARIES
        PUBLIC Core
    QT_LINK_LIBRARIES
        PUBLIC Network
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/qcoronetwork-config.h
    DESTINATION ${QCORO_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

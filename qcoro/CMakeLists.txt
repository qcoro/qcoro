# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/qcoro-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/qcoro-config.h
)

add_qcoro_library(
    NAME Coro
    INTERFACE
    INCLUDEDIR Coro
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
        INTERFACE Core
)

# Install the extra macros file
install(
    FILES "QCoroMacros.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${QCORO_TARGET_PREFIX}Coro/"
    COMPONENT Devel
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/qcoro-config.h
    DESTINATION ${QCORO_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

############################

add_subdirectory(core)

if (QCORO_WITH_QTDBUS)
    add_subdirectory(dbus)
endif()

if (QCORO_WITH_QTNETWORK)
    add_subdirectory(network)
endif()


# SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

# QWebEnginePage has moved from WebEngineWidgets in Qt5 to
# WebEngineCore in Qt6 and we want to copy that.
if (${QT_VERSION_MAJOR} EQUAL 5)
    set(core_SRCS)
    set(core_HDRS)
    set(widgets_SRCS qcorowebenginepage.cpp)
    set(widgets_HDRS QCoroWebEnginePage)
else()
    set(core_SRCS qcorowebenginepage.cpp)
    set(core_HDRS QCoroWebEnginePage)
    set(widgets_SRCS)
    set(widgets_HDRS)
endif()


add_qcoro_library(
    NAME WebEngineCore
    SOURCES
        ${core_SRCS}
    CAMELCASE_HEADERS
        ${core_HDRS}
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
        PUBLIC Core WebEngineCore
)

add_qcoro_library(
    NAME WebEngineWidgets
    SOURCES
        ${widgets_SRCS}
    CAMELCASE_HEADERS
        ${widgets_HDRS}
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
        PUBLIC Core WebEngineWidgets
)

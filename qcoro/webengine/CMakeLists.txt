# SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

# QWebEnginePage has moved from WebEngineWidgets in Qt5 to
# WebEngineCore in Qt6 and we want to copy that.
if (${QT_VERSION_MAJOR} EQUAL 5)
    set(webengine_lib WebEngineWidgets)
else()
    set(webengine_lib WebEngineCore)
endif()


add_qcoro_library(
    NAME WebEngine
    SOURCES
        qcorowebenginepage.cpp
    CAMELCASE_HEADERS
        QCoroWebEnginePage
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
        PUBLIC Core ${webengine_lib}
)

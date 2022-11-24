# SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
# SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME Quick
    SOURCES
        qcoroimageprovider.cpp
    CAMELCASE_HEADERS
        QCoroImageProvider
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
    PUBLIC Core Quick
)

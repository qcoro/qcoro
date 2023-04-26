# SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME Test
    INTERFACE
    CAMELCASE_HEADERS
        QCoroTest
    QT_LINK_LIBRARIES
        INTERFACE Test
)


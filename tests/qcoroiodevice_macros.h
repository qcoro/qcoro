// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

#define QCORO_TEST_IODEVICE_READALL(device)                                                        \
    QByteArray data;                                                                               \
    Q_FOREVER {                                                                                    \
        const auto buf = co_await qCoro((device)).readAll();                                       \
        if (buf.isNull()) {                                                                        \
            break;                                                                                 \
        }                                                                                          \
        data += buf;                                                                               \
    }                                                                                              \
    QCORO_VERIFY(!data.isEmpty());                                                                 \
    QCORO_COMPARE((device).bytesAvailable(), 0)

#define QCORO_TEST_IODEVICE_READ(device)                                                           \
    QByteArray data;                                                                               \
    Q_FOREVER {                                                                                    \
        const auto buf = co_await qCoro((device)).read(1);                                         \
        if (buf.isNull()) {                                                                        \
            break;                                                                                 \
        }                                                                                          \
        data += buf;                                                                               \
    }                                                                                              \
    QCORO_VERIFY(!data.isEmpty());                                                                 \
    QCORO_COMPARE((device).bytesAvailable(), 0)

#define QCORO_TEST_IODEVICE_READLINE(device)                                                       \
    QByteArrayList lines;                                                                          \
    Q_FOREVER {                                                                                    \
        const auto buf = co_await qCoro((device)).readLine();                                      \
        if (buf.isNull()) {                                                                        \
            break;                                                                                 \
        }                                                                                          \
        lines.push_back(buf);                                                                      \
    }                                                                                              \
    QCORO_COMPARE((device).bytesAvailable(), 0)

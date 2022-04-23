// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoroiodevice_p.h"

using namespace QCoro::detail;

WaitSignalHelper::WaitSignalHelper(const QIODevice *device, void(QIODevice::*signalFunc)())
    : QObject()
    , mReady(connect(device, signalFunc, this, [this]() { this->emitReady(true); }))
    , mAboutToClose(connect(device, &QIODevice::aboutToClose, this, [this]() { this->emitReady(false); }))
{}

WaitSignalHelper::WaitSignalHelper(const QIODevice *device, void(QIODevice::*signalFunc)(qint64))
    : QObject()
    , mReady(connect(device, signalFunc, this, &WaitSignalHelper::emitReady<qint64>))
    , mAboutToClose(connect(device, &QIODevice::aboutToClose, this, [this]() { this->emitReady(static_cast<qint64>(0)); }))
{}

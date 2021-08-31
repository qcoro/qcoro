// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "iodevice.h"

#include <QIODevice>

using namespace QCoro::detail;

IODeviceAwaiter::IODeviceAwaiter(QIODevice *device)
    : mDevice(device)
{}

IODeviceAwaiter::IODeviceAwaiter(QIODevice &device)
    : mDevice(&device)
{}

bool IODeviceAwaiter::await_ready() const noexcept {
    return mDevice && (!mDevice->isOpen() || mDevice->bytesAvailable() > 0);
}

void IODeviceAwaiter::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    mConn = QObject::connect(mDevice, &QIODevice::readyRead, [this, awaitingCoroutine]() mutable {
                QObject::disconnect(mConn);
                awaitingCoroutine.resume();
            });
}

QByteArray IODeviceAwaiter::await_resume() const {
    return mDevice->readAll();
}


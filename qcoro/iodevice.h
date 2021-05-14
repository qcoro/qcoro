// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <QIODevice>
#include <QMetaMethod>
#include <QPointer>

/*! \cond internal */

namespace QCoro::detail {

class IODeviceAwaiter {
public:
    explicit IODeviceAwaiter(QIODevice *device) : mDevice(device) {}
    explicit IODeviceAwaiter(QIODevice &device) : mDevice(&device){};

    bool await_ready() const noexcept {
        return mDevice && (!mDevice->isOpen() || mDevice->bytesAvailable() > 0);
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
        mConn =
            QObject::connect(mDevice, &QIODevice::readyRead, [this, awaitingCoroutine]() mutable {
                QObject::disconnect(mConn);
                awaitingCoroutine.resume();
            });
    }

    QByteArray await_resume() const {
        return mDevice->readAll();
    }

private:
    QPointer<QIODevice> mDevice;
    QMetaObject::Connection mConn;
};

template<typename T>
struct awaiter_type<T, std::enable_if_t<std::is_base_of_v<QIODevice, T>>> {
    using type = IODeviceAwaiter;
};
template<typename T>
struct awaiter_type<T *, std::enable_if_t<std::is_base_of_v<QIODevice, T>>> {
    using type = IODeviceAwaiter;
};

} // namespace QCoro::detail

/*! \endcond */

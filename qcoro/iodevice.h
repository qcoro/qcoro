// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QPointer>
#include <QIODevice>
#include <QMetaMethod>

namespace QCoro::detail
{

class IODeviceAwaiter {
public:
    explicit IODeviceAwaiter(QIODevice *socket)
        : mSocket(socket) {}
    explicit IODeviceAwaiter(QIODevice &socket)
        : mSocket(&socket) {};

    bool await_ready() const noexcept {
        return mSocket && mSocket->bytesAvailable() > 0;
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
        mConn = QObject::connect(mSocket, &QIODevice::readyRead,
                                [this, awaitingCoroutine]() mutable {
                                    QObject::disconnect(mConn);
                                    awaitingCoroutine.resume();
                                });
    }

    QByteArray await_resume() const {
        return mSocket->readAll();
    }

private:
    QPointer<QIODevice> mSocket;
    QMetaObject::Connection mConn;
};

template<typename T>
struct awaiter_type<T, std::enable_if_t<std::is_base_of_v<QIODevice, T>>> {
    using type = IODeviceAwaiter;
};
template<typename T>
struct awaiter_type<T*, std::enable_if_t<std::is_base_of_v<QIODevice, T>>> {
    using type = IODeviceAwaiter;
};

} // namespace QCoro::detail

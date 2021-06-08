// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <QMetaMethod>
#include <QPointer>

class QIODevice;

/*! \cond internal */

namespace QCoro::detail {

class IODeviceAwaiter {
public:
    explicit IODeviceAwaiter(QIODevice *device);
    explicit IODeviceAwaiter(QIODevice &device);

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QByteArray await_resume() const;

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

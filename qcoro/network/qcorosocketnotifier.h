// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcorotask.h"
#include "qcoronetwork_export.h"

#include <QPointer>

#include <chrono>
#include <qsocketnotifier.h>

namespace QCoro::detail {

using namespace std::chrono_literals;

class QCORONETWORK_EXPORT QCoroSocketNotifier {
public:
    ///! Constructor.
    explicit QCoroSocketNotifier(QSocketNotifier *server);

    Task<bool> waitForActivated(std::chrono::milliseconds timeout = 30s);

    class WaitForSocketActivatedOperation {
    public:
        explicit WaitForSocketActivatedOperation(QSocketNotifier *notifier, std::chrono::milliseconds timeout = 30s);
        explicit WaitForSocketActivatedOperation(QSocketNotifier &notifier, std::chrono::milliseconds timeout = 30s);
        explicit WaitForSocketActivatedOperation(QSocketNotifier &&notifier, std::chrono::milliseconds timeout = 30s);

        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine);
        bool await_resume() const noexcept;

    private:
        QPointer<QSocketNotifier> mNotifier;
        std::chrono::milliseconds mTimeout;
        bool mResult = false;
    };

private:
    QPointer<QSocketNotifier> mNotifier;
};

} // namespace QCoro::detail

//! Returns a coroutine-friendly wrapper for QTcpServer object.
/*!
 * Returns a wrapper for QTcpServer \c s that provides coroutine-friendly way
 * of co_awaiting new connections.
 *
 * @see docs/reference/qtcpserver.md
 */
inline auto qCoro(QSocketNotifier &s) noexcept {
    return QCoro::detail::QCoroSocketNotifier{&s};
}
//! \copydoc qCoro(QTcpServer &s) noexcept
inline auto qCoro(QSocketNotifier *s) noexcept {
    return QCoro::detail::QCoroSocketNotifier{s};
}

/*! \cond internal */
namespace QCoro::detail {

template<>
struct awaiter_type<QSocketNotifier *> {
    using type = QCoroSocketNotifier::WaitForSocketActivatedOperation;
};

template<>
struct awaiter_type<QSocketNotifier> {
    using type = QCoroSocketNotifier::WaitForSocketActivatedOperation;
};

} // namespace QCoro::detail

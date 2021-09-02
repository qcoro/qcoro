// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <qglobal.h>
#include <QDBusPendingReply>

class QDBusMessage;

/*! \cond internal */

namespace QCoro::detail {

template<typename ... Args>
class QCoroDBusPendingReply {
private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // QDBusPendingReply is a variadic template since Qt6, but in Qt5 the
    // maximum number of template arguments was 8, so we simulate the Qt5
    // behavior here.
    static_assert(sizeof...(Args) <= 8, "In Qt5 QDBusPendingReply has maximum 8 arguments.");
#endif

    class WaitForFinishedOperation {
    public:
        explicit WaitForFinishedOperation(const QDBusPendingReply<Args ...> &reply)
            : mReply(reply)
        {}

        bool await_ready() const noexcept {
            return mReply.isFinished();
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            auto *watcher = new QDBusPendingCallWatcher{mReply};
            QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                             [awaitingCoroutine](auto *watcher) mutable {
                                 awaitingCoroutine.resume();
                                 watcher->deleteLater();
                             });
        }

        QDBusPendingReply<Args ...> await_resume() const {
            Q_ASSERT(mReply.isFinished());
            return mReply;
        }

    private:
        QDBusPendingReply<Args ...> mReply;
    };

    QDBusPendingReply<Args ...> mReply;

    friend struct awaiter_type<QDBusPendingReply<Args ...>>;
public:
    explicit QCoroDBusPendingReply(const QDBusPendingReply<Args ...> &reply)
        : mReply(reply) {}

    WaitForFinishedOperation waitForFinished() {
        return WaitForFinishedOperation{mReply};
    }
};

template<typename ... Args>
struct awaiter_type<QDBusPendingReply<Args ...>> {
    using type = typename QCoroDBusPendingReply<Args ...>::WaitForFinishedOperation;
};

} // namespace QCoro::detail

template<typename ... Args>
inline auto qCoro(const QDBusPendingReply<Args ...> &reply) {
    return QCoro::detail::QCoroDBusPendingReply<Args ...>{reply};
}

/*! \endcond */

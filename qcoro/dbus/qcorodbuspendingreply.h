// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcorotask.h"
#include "qcorosignal.h"

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

        void await_suspend(std::coroutine_handle<> awaitingCoroutine) {
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
    //! Constructor.
    explicit QCoroDBusPendingReply(const QDBusPendingReply<Args ...> &reply)
        : mReply(reply) {}

    /*!
     \brief Operation that allows co_awaiting completion of the pending DBus reply.

     <!-- doc-waitForFinished-start -->
     Waits until the DBus call is finished. This is equivalent to using
     [`QDBusPendingCallWatcher`][qdoc-qdbuspendingcallwatcher] and waiting for it
     to emit the [`finished()`][qdoc-qdbuspendingcallwatcher-finished] signal.

     Returns a `QDBusMessage` representing the received reply. If the reply is already
     finished or an error has occurred the coroutine will not suspend and will return
     a result immediatelly.

     This is a coroutine-friendly equivalent to using [`QDBusPendingCallWatcher`][qdoc-qdbuspendingcallwatcher]:

     ```cpp
     QDBusPendingCall call = interface.asyncCall(...);
     QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
     QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                      this, [](QDBusPendingCallWatcher *watcher) {
                         watcher->deleteLater();
                         const QDBusPendingReply<...> reply = *watcher;
                         ...
                     });
     ```

     It is also possible to just directly use a `QDBusPendingReply` in a `co_await`
     expression to await its completion:
     ```cpp
     QDBusPendingReply<...> pendingReply = interface.asyncCall(...);
     const auto reply = co_await pendingReply;
     ```

     The above is equivalent to:
     ```cpp
     QDBusPendingReply<...> pendingReply = interface.asyncCall(...);
     const auto reply = co_await qCoro(pendingReply).waitForFinished();
     ```

     [qdoc-qdbuspendingcallwatcher]: https://doc.qt.io/qt-5/qdbuspendingcallwatcher.html
     [qdoc-qdbuspendingcallwatcher-finished]: https://doc.qt.io/qt-5/qdbuspendingcallwatcher.html#finished

     <!-- doc-waitForFinished-end -->

     @see docs/reference/qdbuspendingreply.md
     */
    Task<QDBusPendingReply<Args ...>> waitForFinished() {
        if (!mReply.isFinished()) {
            QDBusPendingCallWatcher watcher{mReply};
            co_await qCoro(&watcher, &QDBusPendingCallWatcher::finished);
            co_return watcher.reply();
        }
        co_return mReply;
    }
};

template<typename ... Args>
struct awaiter_type<QDBusPendingReply<Args ...>> {
    using type = typename QCoroDBusPendingReply<Args ...>::WaitForFinishedOperation;
};

} // namespace QCoro::detail

/*! \endcond */

//! Returns a coroutine-friendly wrapper for a QDBusPendingReply object.
/*!
 * Returns a wrapper for the QDBusPendingReply \c reply that provides
 * a coroutine-friendly way to await the completion of the pending reply.
 *
 * Note that is is also possible to just directly `co_await` the `QDBusPendingReply`
 * completion without using the wrapper class:
 *
 * ```
 * QDBusPendingReply<...> pendingReply = interface.asyncCall(...);
 * const auto reply = co_await pendingReply;
 * ```
 *
 * @see docs/reference/qdbuspendingreply.md
 */
template<typename ... Args>
inline auto qCoro(const QDBusPendingReply<Args ...> &reply) {
    return QCoro::detail::QCoroDBusPendingReply<Args ...>{reply};
}

// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcorotask.h"
#include "qcorodbus_export.h"

#include <QDBusPendingCallWatcher>

class QDBusMessage;
class QDBusPendingCall;

/*! \cond internal */

namespace QCoro::detail {

class QCORODBUS_EXPORT QCoroDBusPendingCall {
private:
    class WaitForFinishedOperation {
    public:
        explicit WaitForFinishedOperation(const QDBusPendingCall &call);

        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
        QDBusMessage await_resume() const;
    private:
        const QDBusPendingCall &mCall;
    };

    const QDBusPendingCall &mCall;

    friend struct awaiter_type<QDBusPendingCall>;
public:
    //! Constructor.
    explicit QCoroDBusPendingCall(const QDBusPendingCall &call);

    /*!
     \brief Operation that allows co_awaiting completion of the pending DBus call.

     <!-- doc-waitForFinished-start -->
     Waits until the DBus call is finished. This is equivalent to using
     [`QDBusPendingCallWatcher`][qdoc-qdbuspendingcallwatcher] and waiting for it
     to emit the [`finished()`][qdoc-qdbuspendingcallwatcher-finished] signal.

     Returns a `QDBusMessage` representing the reply to the call.

     If the call is already finished or has an error, the coroutine will not suspend and the `co_await`
     expression will return immediatelly.

     It is also possible to just directly use a `QDBusPendingCall` in a `co_await`
     expression to await its completion:
     ```cpp
     QDBusPendingCall pendingCall = interface.asyncCall(...);
     const auto reply = co_await pendingCall;
     ```

     The above is equivalent to:
     ```cpp
     QDBusPendingCall pendingCall = interface.asyncCall(...);
     const auto reply = co_await qCoro(pendingCall).waitForFinished();
     ```

     This is a coroutine-friendly equivalent to using [`QDBusPendingCallWatcher`][qdoc-qdbuspendingcallwatcher]:

     ```cpp
     QDBusPendingCall call = interface.asyncCall(...);
     QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call);
     QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                      this, [](QDBusPendingCallWatcher *watcher) {
                         watcher->deleteLater();
                         const QDBusReply<...> reply = *watcher;
                         ...
                     });
     ```

     [qdoc-qdbuspendingcallwatcher]: https://doc.qt.io/qt-5/qdbuspendingcallwatcher.html
     [qdoc-qdbuspendingcallwatcher-finished]: https://doc.qt.io/qt-5/qdbuspendingcallwatcher.html#finished
     <!-- doc-waitForFinished-end -->

     @see docs/reference/qdbuspendingcall.md
     */
    Task<QDBusMessage> waitForFinished();
};

template<>
struct awaiter_type<QDBusPendingCall> {
    using type = QCoroDBusPendingCall::WaitForFinishedOperation;
};

} // namespace QCoro::detail

/*! \endcond */

//! Returns a co_await-friendly wrapper for QDBusPendingCall object
/*!
 * Returns a wrapper for QDBusPendingCall \c call that provides coroutine-friendly
 * way to co_await completion of the call.
 *
 * @see docs/reference/qdbuspendingcall.md
 */
inline auto qCoro(const QDBusPendingCall &call) {
    return QCoro::detail::QCoroDBusPendingCall{call};
}

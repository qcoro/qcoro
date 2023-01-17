// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QPointer>

#include "qcorocore_export.h"
#include "qcoro/coroutine.h"

#include <chrono>

class QThread;

namespace QCoro {
template<typename T>
class Task;

namespace detail {
class ThreadContextPrivate;
} // namespace detail

class ThreadContext {
public:
    explicit ThreadContext(QThread *thread);
    ~ThreadContext();
    ThreadContext(const ThreadContext &) = delete;
    ThreadContext &operator=(const ThreadContext &) = delete;
#ifdef Q_CC_GNU
    // Workaround for the a GCC bug(?) where GCC tries to move the ThreadContext
    // into QCoro::TaskPromise::await_transform() (most likely).
    ThreadContext(ThreadContext &&) noexcept;
#else
    ThreadContext(ThreadContext &&) = delete;
#endif
    ThreadContext &operator=(ThreadContext &&) = delete;

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> awaiter) noexcept;
    void await_resume() noexcept;

private:
    std::unique_ptr<detail::ThreadContextPrivate> d;
};

ThreadContext moveToThread(QThread *thread);

} // namespace QCoro

namespace QCoro::detail {

class QCOROCORE_EXPORT QCoroThread {
public:
    explicit QCoroThread(QThread *thread);

    /**
     * \brief Coroutine that waits for a thread to get started.
     *
     * \return Returns `true` when the thread is already running or when it starts within the
     * specified timeout. If the thread has already finished, or the operation times out, the
     * coroutine returns `false`.
     *
     * If the timeout is -1 the operation will never time out.
     *
     * See [`QThread::started()`][qtdoc-qthread-started] documentation for details.
     *
     * [qtdoc-qthread-started]: https://doc.qt.io/qt-5/qthread.html#started
     */
    Task<bool> waitForStarted(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

    /**
     * \brief Coroutine that waits for a thread to finish.
     *
     * \return Returns `true` when the thread has already finished or when it finishes within
     * specified timeout. If the thread is not running and hasn't finished yet, or when the
     * operation times out, the coroutine returns `false`.
     *
     * If the timeout is -1 the operation will never time out.
     *
     * See [`QThread::finished()`][qtdoc-qthread-finished] documentation for details.
     *
     * [qdoc-qthread-finished]: https://doc.qt.io/qt-5/qthread.html#finished
     */
    Task<bool> waitForFinished(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

private:
    QPointer<QThread> mThread;
};

} // namespace QCoro::detail

inline auto qCoro(QThread *thread) {
    return QCoro::detail::QCoroThread(thread);
}

inline auto qCoro(QThread &thread) {
    return QCoro::detail::QCoroThread(&thread);
}

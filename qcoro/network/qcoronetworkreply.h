// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroiodevice.h"
#include "qcoronetwork_export.h"

#include <QNetworkReply>

namespace QCoro::detail {

class QCORONETWORK_EXPORT QCoroNetworkReply final : public QCoroIODevice {
private:
    class WaitForFinishedOperation final {
    public:
        explicit WaitForFinishedOperation(QPointer<QNetworkReply> reply);
        ~WaitForFinishedOperation();

        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine);
        QNetworkReply *await_resume() const noexcept;

    private:
        struct Private;
        std::unique_ptr<Private> d;
        // BC: This class used to have a QPointer<QNetworkReply>, which is 2*sizeof(void*),
        // while std::unique_ptr is only sizeof(void*), so this dummy one is to ensure binary
        // compatibility of this class.
        // FIXME: Remove in 1.0
        // Silly gcc 11, cannot detect the dummy is unused and warns about unused attribute
        #if __GNUC__ > 11 || defined(__clang__) || defined(_MSC_VER)
        [[maybe_unused]]
        #endif
        void *dummy = nullptr;
    };

    friend struct awaiter_type<QNetworkReply *>;
public:
    using QCoroIODevice::QCoroIODevice;

    /**
     * \brief Waits for the reply to finish.
     *
     * Waits for at most \c timeout milliseconds. If the \c timeout is -1, the call will never
     * time out.
     *
     * \return Returns `true` if the reply has finished (with or without an error), `false` if
     * the wait has timed out.
     */
    Task<bool> waitForFinished(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

private:
    Task<std::optional<bool>> waitForReadyReadImpl(std::chrono::milliseconds timeout) override;
    Task<std::optional<qint64>> waitForBytesWrittenImpl(std::chrono::milliseconds timeout) override;
};

} // namespace QCoro::detail

//! Returns a coroutine-friendly wrapper for QNetworkReply object.
/*!
 * Returns a wrapper for the QNetworkReply \c s that provides coroutine-friendly
 * way to co_await read and write operations.
 *
 * @see docs/reference/qnetworkreply.md
 */
inline auto qCoro(QNetworkReply &s) noexcept {
    return QCoro::detail::QCoroNetworkReply{&s};
}
//! \copydoc qCoro(QAbstractSocket &s) noexcept
inline auto qCoro(QNetworkReply *s) noexcept {
    return QCoro::detail::QCoroNetworkReply{s};
}

/*! \cond internal */
namespace QCoro::detail {

template<>
struct awaiter_type<QNetworkReply *> {
    using type = QCoroNetworkReply::WaitForFinishedOperation;
};

} // namespace QCoro::detail

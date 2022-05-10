// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"
#include "qcorowebsockets_export.h"

#include <QWebSocketProtocol>

#include <tuple>
#include <chrono>
#include <optional>

class QWebSocket;
class QNetworkRequest;
class QUrl;

namespace QCoro::detail {

class QCOROWEBSOCKETS_EXPORT QCoroWebSocket {
public:
    explicit QCoroWebSocket(QWebSocket *websocket);

    Task<bool> open(const QUrl &url, std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});
    Task<bool> open(const QNetworkRequest &request, std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

    Task<std::optional<qint64>> ping(const QByteArray &payload, std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

    Task<std::optional<std::tuple<QByteArray, bool>>> binaryFrame(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});
    Task<std::optional<QByteArray>> binaryMessage(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

    Task<std::optional<std::tuple<QString, bool>>> textFrame(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});
    Task<std::optional<QString>> textMessage(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

private:
    QWebSocket *mWebSocket;
};

} // namespace QCoro::detail

auto inline qCoro(QWebSocket *websocket) noexcept {
    return QCoro::detail::QCoroWebSocket{websocket};
}

auto inline qCoro(QWebSocket &websocket) noexcept {
    return QCoro::detail::QCoroWebSocket{&websocket};
}

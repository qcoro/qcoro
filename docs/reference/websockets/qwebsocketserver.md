<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QWebSocketServer

{{ doctable("WebSockets", "QCoroWebSocketServer") }}

QCoro provides a wrapper for the [`QWebSocketServer`][qtdoc-qwebsocketserver] class which allows
user to asynchronously co_await incoming connections. To wrap a `QWebSocketServer` instance, use
the `qCoro()` overload from the `QCoroWebSocketServer` include header.


```cpp
QCoroWebSocketServer qCoro(QWebSocketServer &);
QCoroWebSocketServer qCoro(QWebSocketServer *);
```

## nextPendingConnection()

Suspends the awaiter until a new incoming connection becomes available or until the specified timeout.
If the specified timeout is `-1`, the operation will never time out.

Returns a pointer to [`QWebSocket`][qtdoc-qwebsocket] representing the new connection, or a null pointer if the operation
times out, the server is not [`listen()`][qtdoc-qwebsocketserver-listen]ining for incoming connections.

```cpp
QCoro::Task<QWebSocket *> QCoroWebSocketServer::nextPendingConnection(std::chrono::milliseconds timeout);
```

Note that [`pauseAccepting()`][qtdoc-qwebsocketserver-pauseAccepting] doesn't resume any awaiting
coroutines.

```cpp
QCoro::Task<> listen(QWebSocketServer *server) {
    server->listen();
    while (auto socket = std::unique_ptr<QWebSocket>(co_await qCoro(server).nextPendingConnection());
           socket != nullptr)
    {
        handleConnection(std::move(socket));
    }
}
```

[qtdoc-qwebsocketserver]: https://doc.qt.io/qt-5/qwebsocketserver.html
[qtdoc-qwebsocketserver-pauseAccepting]: https://doc.qt.io/qt-5/qwebsocketserver.html#pauseAccepting
[qtdoc-qwebsocketserver-listen]: https://doc.qt.io/qt-5/qwebsocketserver.html#listen
[qtdoc-qwebsocket]: https://doc.qt.io/qt-5/qwebsocket.html

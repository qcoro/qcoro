<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QWebSocket

{{ doctable("WebSockets", "QCoroWebSocket") }}


[`QWebSocket`][qtdoc-qwebsocket] provides a client socket to connect to a WebSocket server, which
several asynchronous operations that could be used as coroutines. The `QCoroWebSocket` wrapper
provides exactly this. Use `qCoro()` to wrap an existing instance of `QWebSocket` to become
`QCoroWebSocket`:

```cpp
QCoroWebSocket qCoro(QWebSocket &);
QCoroWebSocket qCoro(QWebSocket *);
```

## open()

Opens connection to the WebSocket server and waits until the connection is established, or
a timeout occurs. Resolves to `true` when the connection was successfully established, or
`false` if the process has timed out. If the timeout is `-1`, the operation will never time out.

See documentation for [`QWebSocket::open(const QUrl &)`][qtdoc-qwebsocket-open-qurl] and
[`QWebSocket::open(const QNetworkRequest &)`][qtdoc-qwebsocket-open-qnetworkrequest] for details.

```cpp
QCoro::Task<bool> QCoroWebSocket::open(const QUrl &url, std::chrono::milliseconds timeout);
QCoro::Task<bool> QCoroWebSocket::open(const QNetworkRequest &request, std::chrono::milliseconds timeout);
```

## ping()

Sends the given payload to the server and waits for response. Returns the roundtrip time
elapsed, or empty value if the operation has timed out. If the timeout is set to `-1`, the
operation will never time out.

See documentation for [`QWebSocket::ping()`][qtdoc-qwebsocket-ping] for details.

```cpp
QCoro::Task<std::optional<std::chrono::milliseconds>> ping(const QByteArray &payload, std::chrono::milliseconds timeout);
```

## binaryFrames()

Returns an [asynchronous generator][qcoro-async-generator] that will yield frame data whenever
they arrive. More specifically, the generator will yield a tuple of `QByteArray` containing the
frame data, and a `boolean` value, indicating whether this is the last frame of a message,
allowing to receive large messages in smaller frame-sized chunks.

Note that the generator will never terminate, unless the socket is disconnected or unless the
time elapsed between frames gets over the specified `timeout`. If the `timeout` is `-1`, the
generator will wait for new frames indefinitely.

See documentation for the [`QWebSocket::binaryFrameReceived()`][qtdoc-qwebsocket-binary-frame-received] signal for details.

```cpp
QCoro::AsyncGenerator<std::tuple<QByteArray, bool>> QCoroWebSocket::binaryFrames(std::chrono::milliseconds timeout);
```

!!! question "Why generator instead of simply co_awaiting next frame?"
    It's logical to ask why does this function return a generator, rather than simply
    returning the received frame, the user would then go on to `co_await` the next
    frame etc. Unlike e.g. `QIODevice`-based classes, the `QWebSocket` is not buffered,
    that is any frame that arrives when no-one is connected to the `binaryFrameReceived()`
    signal is simply dropped and cannot be retrieved later. Therefore we need to use a
    generator API, which will buffer all received frames for as long as the generator
    object exists and provide them through the familiar iterator-like interface.


Here's an example coroutine that assembles messages from incoming frames and emits a singal
whenever a full message is assembled (don't use this in real code, use the `binaryMessages()`
coroutine to receive complete messages).

```cpp
QCoro::Task<> MessageReceived::receive() {
    QByteArray message;
    QCORO_FOREACH(const auto &[frame, isLast], qCoro(mWebSocket).binaryFrames()) {
        message.append(frame);
        if (isLast) {
            Q_EMIT messageAssembled(message);
            message.clear();
        }
    }
    qDebug() << "Socket disconnected!";
}

```

## textFrames()

Behaves exactly like [`binaryFrames()`](#binaryframes), except that it expects the frame
to contain text data.

```cpp
QCoro::AsyncGenerator<std::tuple<QString, bool>> QCoroWebSocket::textFrames(std::chrono::milliseconds timeout);
```

## binaryMessages()

Returns an [asynchronous generator][qcoro-async-generator] that will yield a binary message whenever
it arrives.

Note that the generator will never terminate, unless the socket is disconnected or unless the
time elapsed between frames gets over the specified `timeout`. If the `timeout` is `-1`, the
generator will wait for new frames indefinitely.

See documentation for the [`QWebSocket::binaryMessageReceived()`][qtdoc-qwebsocket-binary-message-received] signal for details.

```cpp
QCoro::AsyncGenerator<QByteArray> QCoroWebSocket::binaryMessages(std::chrono::milliseconds timeout);
```

## textMessages()

Behaves exactly like [`binaryMessages()`](#binarymessages), except that it expects the message
to be text message.


```cpp
QCoro::AsyncGenerator<QString> QCoroWebSocket::textMessage(std::chrono::milliseconds timeout);
```


[qtdoc-qwebsocket]: https://doc.qt.io/qt-5/qwebsocket.html
[qtdoc-qwebsocket-open-qurl]: https://doc.qt.io/qt-5/qwebsocket.html#open
[qtdoc-qwebsocket-open-qnetworkrequest]: https://doc.qt.io/qt-5/qwebsocket.html#open-1
[qtdoc-qwebsocket-ping]: https://doc.qt.io/qt-5/qwebsocket.html#ping

[qcoro-async-generator]: ../coro/asyncgenerator.md

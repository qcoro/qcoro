<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QLocalSocket

{{ doctable("Network", "QCoroLocalSocket", ("core/qiodevice", "QCoroIODevice")) }}

[`QLocalSocket`][qtdoc-qlocalsocket] has several potentially asynchronous operations
in addition to reading and writing, which are provided by [`QIODevice`][qtdoc-qiodevice]
baseclass and can be used with coroutines thanks to QCoro's [`QCoroIODevice`][qcoro-qcoroiodevice].
Those operations are connecting to and disconnecting from the server.

Since `QLocalSocket` doesn't provide the ability to `co_await` those operations, QCoro provides
 a wrapper calss `QCoroLocalSocket`. To wrap a `QLocalSocket` object into the `QCoroLocalSocket`
 wrapper, use [`qCoro()`][qcoro-coro]:

```cpp
QCoroLocalSocket qCoro(QLocalSocket &);
QCoroLocalSocket qCoro(QLocalSocket *);
```

Same as `QLocalSocket` is a subclass of `QIODevice`, `QCoroLocalSocket` subclasses
[`QCoroIODevice`][qcoro-qcoroiodevice], so it also provides the awaitable interface for selected
`QIODevice` functions. See [`QCoroIODevice`][qcoro-qcoroiodevice] documentation for details.

## `waitForConnected()`

Waits for the socket to connect or until it times out. Returns `bool` indicating whether
connection has been established (`true`) or whether the operation has timed out or another
error has occurred (`false`). Returns immeditelly if the socket is already in connected
state.

If the timeout is -1, the operation will never time out.

See documentation for [`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected]
for details.

```cpp
QCoro::Task<bool> QCoroLocalSocket::waitForConnected(int timeout_msecs = 30'000);
QCoro::Task<bool> QCoroLocalSocket::waitForConnected(std::chrono::milliseconds timeout);
```

## `waitForDisconnected()`

Waits for the socket to disconnect from the server or until the operation times out.
Returns immediatelly if the socket is not in connected state.

If the timeout is -1, the operation will never time out.

See documentation for [`QLocalSocket::waitForDisconnected()`][qtdoc-qlocalsocket-waitForDisconnected]
for details.

```cpp
QCoro::Task<bool> QCoroLocalSocket::waitForDisconnected(timeout_msecs = 30'000);
QCoro::Task<bool> QCoroLocalSocket::waitForDisconnected(std::chrono::milliseconds timeout);
```

## `connectToServer()`

`QCoroLocalSocket` provides an additional method called `connectToServer()` which is equivalent
to calling `QLocalSocket::connectToServer()` followed by `QLocalSocket::waitForConnected()`. This
operation is co_awaitable as well.

If the timeout is -1, the operation will never time out.

See the documentation for [`QLocalSocket::connectToServer()`][qtdoc-qlocalsocket-connectToServer] and
[`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected] for details.

```cpp
QCoro::Task<bool> QCoroLocalSocket::connectToServer(QIODevice::OpenMode openMode = QIODevice::ReadOnly,
                                                    std::chrono::milliseconds timeout = std::chrono::seconds(30));
QCoro::Taks<bool> QCoroLocalSocket::connectToServer(const QString &name, QIODevice::OpenMode openMode = QIODevice::ReadOnly,
                                                    std::chrono::milliseconds timeout = std::chrono::seconds(30));
```

## Examples

```cpp
QCoro::Task<QByteArray> requestDataFromServer(const QString &serverName) {
    QLocalSocket socket;
    if (!co_await qCoro(socket).connectToServer(serverName)) {
        qWarning() << "Failed to connect to the server";
        co_return QByteArray{};
    }

    socket.write("SEND ME DATA!");

    QByteArray data;
    while (!data.endsWith("\r\n.\r\n")) {
        data += co_await qCoro(socket).readAll();
    }

    co_return data;
}
```

[qtdoc-qiodevice]: https://doc.qt.io/qt-5/qiodevice.html
[qtdoc-qlocalsocket]: https://doc.qt.io/qt-5/qlocalsocket.html
[qtdoc-qlocalsocket-connectToServer]: https://doc.qt.io/qt-5/qlocalsocket.html#connectToServer
[qtdoc-qlocalsocket-waitForConnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForConnected
[qtdoc-qlocalsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForDisconnected
[qcoro-coro]: ../coro/coro.md
[qcoro-qcoroiodevice]: ../core/qiodevice.md

<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QAbstractSocket

{{ doctable("Network", "QCoroAbstractSocket", ("core/qiodevice", "QCoroIODevice")) }}

[`QAbstractSocket`][qtdoc-qabstractsocket] is a base class for [`QTcpSocket`][qtdoc-qtcpsocket]
and [`QUdpSocket`][qtdoc-qudpsocket] and has some potentially asynchronous operations.
In addition to reading and writing, which are provided by [`QIODevice`][qtdoc-qiodevice]
baseclass and can be used with coroutines thanks to QCoro's [`QCoroIODevice`][qcoro-qcoroiodevice]
it provides asynchronous waiting for connecting to and disconnecting from the server.

Since `QAbstractSocket` doesn't provide the ability to `co_await` those operations, QCoro provides
 a wrapper calss `QCoroAbstractSocket`. To wrap a `QAbstractSocket` object into the `QCoroAbstractSocket`
 wrapper, use [`qCoro()`][qcoro-coro]:

```cpp
QCoroAbstractSocket qCoro(QAbstractSocket &);
QCoroAbstractSocket qCoro(QAbstractSocket *);
```

Same as `QAbstractSocket` is a subclass of `QIODevice`, `QCoroAbstractSocket` subclasses
[`QCoroIODevice`][qcoro-qcoroiodevice], so it also provides the awaitable interface for selected
`QIODevice` functions. See [`QCoroIODevice`][qcoro-qcoroiodevice] documentation for details.

## `waitForConnected()`

Waits for the socket to connect or until it times out. Returns `bool` indicating whether
connection has been established (`true`) or whether the operation has timed out or another
error has occurred (`false`). Returns immediatelly when the socket is already in connected
state.

If the timeout is -1, the operation will never time out.

See documentation for [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected]
for details.

```cpp
QCoro::Task<bool> QCoroAbstractSocket::waitForConnected(int timeout_msecs = 30'000);
QCoro::Task<bool> QCoroAbstractSocket::waitForConnected(std::chrono::milliseconds timeout);
```

## `waitForDisconnected()`

Waits for the socket to disconnect from the server or until the operation times out.
Returns immediatelly if the socket is not in connected state.

If the timeout is -1, the operation will never time out.

See documentation for [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected]
for details.

```cpp
QCoro::Task<bool> QCoroAbstractSocket::waitForDisconnected(timeout_msecs = 30'000);
QCoro::Task<bool> QCoroAbstractSocket::waitForDisconnected(std::chrono::milliseconds timeout);
```

## `connectToHost()`

`QCoroAbstractSocket` provides an additional method called `connectToHost()` which is equivalent
to calling `QAbstractSocket::connectToHost()` followed by `QAbstractSocket::waitForConnected()`. This
operation is co_awaitable as well.

If the timeout is -1, the operation will never time out.

See the documentation for [`QAbstractSocket::connectToHost()`][qtdoc-qabstractsocket-connectToHost] and
[`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected] for details.

```cpp
QCoro::Task<bool> QCoroAbstractSocket::connectToHost(const QHostAddress &address, quint16 port,
                                                     QIODevice::OpenMode openMode = QIODevice::ReadOnly,
                                                     std::chrono::milliseconds timeout = std::chrono::seconds(30));
QCoro::Task<bool> QCoroAbstractSocket::connectToHost(const QString &hostName, quint16 port,
                                                     QIODevice::OpenMode openMode = QIODevice::ReadOnly,
                                                     QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol,
                                                     std::chrono::milliseconds timeout = std::chrono::seconds(30));
```

## Examples

```cpp
{% include "../../examples/qtcpsocket.cpp" %}
```

[qtdoc-qiodevice]: https://doc.qt.io/qt-5/qiodevice.html
[qtdoc-qtcpsocket]: https://doc.qt.io/qt-5/qtcpsocket.html
[qtdoc-qudpsocket]: https://doc.qt.io/qt-5/qudpsocket.html
[qtdoc-qabstractsocket]: https://doc.qt.io/qt-5/qabstractsocket.html
[qtdoc-qabstractsocket-connectToServer]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToServer
[qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
[qtdoc-qabstractsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForDisconnected
[qcoro-coro]: ../coro/coro.md
[qcoro-qcoroiodevice]: ../core/qiodevice.md

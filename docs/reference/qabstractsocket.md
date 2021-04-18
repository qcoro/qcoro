# QAbstractSocket

```cpp
class QCoroAbstractSocket : public QCoroIODevice;
```

[`QAbstractSocket`][qtdoc-qabstractsocket] is a base class for [`QTcpSocket`][qtdoc-qtcpsocket]
and [`QUdpSocket`][qtdoc-qudpsocket] and has some potentially asynchronous operations.
In addition to reading and writing, which are provided by [`QIODevice`][qtdoc-qiodevice]
baseclass and can be used with coroutines thanks to QCoro's [`QCoroIODevice`][qcoro-qcoroiodevice].
Those operations are connecting to and disconnecting from the server.

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
connection has been established or whether the operation has timed out. The coroutine
is not suspended if the socket is already connected.

See documentation for [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected]
for details.

```cpp
Awaitable auto QCoroAbstractSocket::waitForConnected(int timeout_msecs = 30'000);
Awaitable auto QCoroAbstractSocket::waitForConnected(std::chrono::milliseconds timeout);
```

## `waitForDisconnected()`

Waits for the socket to disconnect from the server or until the operation times out.
The coroutine is not suspended if the socket is already disconnected.

See documentation for [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected]
for details.

```cpp
Awaitable auto QCoroAbstractSocket::waitForDisconnected(timeout_msecs = 30'000);
Awaitable auto QCoroAbstractSocket::waitForDisconnected(std::chrono::milliseconds timeout);
```

## `connectToHost()`

`QCoroAbstractSocket` provides an additional method called `connectToHost()` which is equivalent
to calling `QAbstractSocket::connectToHost()` followed by `QAbstractSocket::waitForConnected()`. This
operation is co_awaitable as well.

See the documentation for [`QAbstractSocket::connectToHost()`][qtdoc-qabstractsocket-connectToHost] and
[`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected] for details.

```cpp
Awaitable auto QCoroAbstractSocket::connectToHost(const QHostAddress &address, quint16 port,
                                                  QIODevice::OpenMode openMode = QIODevice::ReadOnly);
Awaitable auto QCoroAbstractSocket::connectToHost(const QString &hostName, quint16 port,
                                                  QIODevice::OpenMode openMode = QIODevice::ReadOnly,
                                                  QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol);
```

## Examples

```cpp
QCoro::Task<QByteArray> requestDataFromServer(const QString &hostName) {
    QTcpSocket socket;
    if (!co_await qCoro(socket).connectToHost(hostName)) {
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
[qtdoc-qtcpsocket]: https://doc.qt.io/qt-5/qtcpsocket.html
[qtdoc-qudpsocket]: https://doc.qt.io/qt-5/qudpsocket.html
[qtdoc-qabstractsocket]: https://doc.qt.io/qt-5/qabstractsocket.html
[qtdoc-qabstractsocket-connectToServer]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToServer
[qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
[qtdoc-qabstractsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForDisconnected
[qcoro-coro]: coro.md
[qcoro-qcoroiodevice]: qiodevice.md

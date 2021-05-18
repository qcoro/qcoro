# QTcpServer

```cpp
class QCoroTcpServer : public QTcpServer;
```

[`QTcpServer`][qtdoc-qtcpserver] really only has one asynchronous operation worth `co_await`ing, and that's
`waitForNewConnection()`.

Since `QTcpServer` doesn't provide the ability to `co_await` those operations, QCoro provides
 a wrapper class `QCoroTcpServer`. To wrap a `QTcpServer` object into the `QCoroTcpServer`
 wrapper, use [`qCoro()`][qcoro-coro]:

```cpp
QCoroTcpServer qCoro(QTcpServer &);
QCoroTcpServer qCoro(QTcpServer *);
```

## `waitForNewConnection()`

Waits until a new incoming connection is available or until it times out. Returns pointer to `QTcpSocket` or
`nullptr` if the operation timed out or another error has occured.

See documentation for [`QTcpServer::waitForNewConnection()`][qtdoc-qtcpserver-waitForNewConnection]
for details.

```cpp
Awaitable auto QCoroTcpServer::waitForNewConnection(int timeout_msecs = 30'000);
Awaitable auto QCoroTcpServer::waitForNewConnection(std::chrono::milliseconds timeout);
```

## Examples

```cpp
QCoro::Task<> runServer(uint16_t port) {
    QTcpServer server;
    server.listen(QHostAddress::LocalHost, port);

    while (server.isListening()) {
        auto *socket = co_await qCoro(server).waitForNewConnection(10s);
        if (socket != nullptr) {
            newClientConnection(socket);
        }
    }
}
```


[qtdoc-qtcpserver]: https://doc.qt.io/qt-5/qtcpserver.html
[qtdoc-qtcpserver-waitForNewConnection]: https://doc.qt.io/qt-5/qtcpserver.html#waitForNewConnection
[qcoro-coro]: coro.md

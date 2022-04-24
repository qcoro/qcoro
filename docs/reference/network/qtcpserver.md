<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QTcpServer

{{ doctable("Network", "QCoroTcpServer") }}

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

If the timeout is -1 the operation will never time out.

See documentation for [`QTcpServer::waitForNewConnection()`][qtdoc-qtcpserver-waitForNewConnection]
for details.

```cpp
QCoro::Task<QTcpSocket *> QCoroTcpServer::waitForNewConnection(int timeout_msecs = 30'000);
QCoro::Task<QTcpSocket *> QCoroTcpServer::waitForNewConnection(std::chrono::milliseconds timeout);
```

## Examples

```cpp
{% include "../../examples/qtcpserver.cpp" %}
```

[qtdoc-qtcpserver]: https://doc.qt.io/qt-5/qtcpserver.html
[qtdoc-qtcpserver-waitForNewConnection]: https://doc.qt.io/qt-5/qtcpserver.html#waitForNewConnection
[qcoro-coro]: ../coro/coro.md

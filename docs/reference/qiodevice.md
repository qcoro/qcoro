# QIODevice

The QCoro frameworks allows `co_await`ing on [QIODevice][qdoc-qiodevice] object. The
co-awaiting coroutine is suspended until the `QIODevice` emits the
[`readyRead()`][qdoc-qiodevice-readyread] signal.

If the `QIODevice` is not opened or there are already some data available to be
read the coroutine is not suspended. To result of the `co_await` operation is the
same as calling [`QIODevice::readAll()`][qdoc-qiodevice-readall]. 

To make it work, include `qcoro/qiodevice.h` in your implementation.

```cpp
#include <qcoro/qiodevice.h>

QCoro::Task<int> MyClass::measureLatency(QTcpSocket *socket) {
    socket->write("PING");
    socket->flush();
    QElapsedTimer timer;
    timer.start();
    const auto result = co_await socket;
    if (result == "PONG") {
        co_return timer.msecsElapsed();
    }

    co_return -1;
}
```

[qdoc-qiodevice]: https://doc.qt.io/qt-5/qiodevice.html
[qdoc-qiodevice-readyread]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
[qdoc-qiodevice-readall]: https://doc.qt.io/qt-5/qiodevice.html#readAll


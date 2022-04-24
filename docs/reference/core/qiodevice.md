<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QIODevice

{{
    doctable("Core", "QCoroIODevice", None,
            [('network/qabstractsocket', 'QCoroAbstractSocket'),
             ('network/qlocalsocket', 'QCoroLocalSocket'),
             ('network/qnetworkreply', 'QCoroNetworkReply'),
             ('core/qprocess', 'QCoroProcess')])
}}
```cpp
class QCoroIODevice
```

[`QIODevice`][qtdoc-qiodevice] has several different IO operations that can be waited on
asynchronously. Since `QIODevice` itself doesn't provide the abaility to `co_await` those
operations, QCoro provides a wrapper class called `QCoroIODevice`. To wrap a `QIODevice`
into a `QCoroIODevice`, use [`qCoro()`][qcoro-coro]:

```cpp
QCoroIODevice qCoro(QIODevice &);
QCoroIODevice qCoro(QIODevice *);
```

Note that Qt provides several subclasses of `QIODevice`. QCoro provides coroutine-friendly
wrappers for some of those types as well (e.g. for [`QLocalSocket`][qlocalsocket]). This
subclass can be passed to `qCoro()` function as well. Oftentimes the wrapper class
will provide some additional features (like co_awaiting establishing connection etc.).
You can check whether QCoro supports the QIODevice subclass by checking the list of supported
Qt types.

## `readAll()`

Waits until there are any data to be read from the device (similar to waiting until the device
emits [`QIODevice::readyRead()`][qtdoc-qiodevice-readyread] signal) and then returns all data
available in the buffer as a `QByteArray`. Doesn't suspend the coroutine if there are already
data available in the `QIODevice` or if the `QIODevice` is not opened for reading.

This is the default operation when `co_await`ing an instance of a `QIODevice` directly. Thus,
it is possible to just do

```cpp
const QByteArray content = co_await device;
```

instead of

```cpp
const QByteArray content = qCoro(device).readAll();
```

See documentation for [`QIODevice::readAll()`][qtdoc-qiodevice-readall] for details.

`QCoroIODevice::readAll()` additionally accepts an optional timeout parameter. If no data
become available within the timeout, the coroutine returns an empty `QByteArray`. If no
timeout is specified or if it is set to `-1`, the operation will never time out.

```cpp
QCoro::Task<QByteArray> QCoroIODevice::readAll();
QCoro::Task<QByteArray> QCoroIODevice::readAll(std::chrono::milliseconds timeout);
```

## `read()`

Waits until there are any data to be read from the device (similar to waiting until the device
emits [`QIODevice::readyRead()`][qtdoc-qiodevice-readyread] signal) and then returns up to
`maxSize` bytes as a `QByteArray`. Doesn't suspend the coroutine if there are already data
available in the `QIODevice` or if the device is not opened for reading.

See documentation for [`QIODevice::read()`][qtdoc-qiodevice-read] for details.

`QCoroIODevice::read()` additionally accepts an optional timeout parameter. If no data
become available within the timeout, the coroutine returns an empty `QByteArray`. If no
timeout is specified or if it is set to `-1`, the operation will never time out.

```cpp
QCoro::Task<QByteArray> QCoroIODevice::read(qint64 maxSize = 0);
QCoro::Task<QByteArray> QCoroIODevice::read(qint64 maxSize, std::chrono::milliseconds timeout);
```

## `readLine()`

Repeatedly waits for data to arrive until it encounters a newline character, end-of-data or
until it reads `maxSize` bytes. Returns the resulting data as `QByteArray`.

See documentation for [`QIODevice::readLine()`][qtdoc-qiodevice-readline] for details.

`QCoroIODevice::readLine()` additionally accepts an optional timeout parameter. If no data
become available within the timeout, the coroutine returns an empty `QByteArray`. If no
timeout is specified or if it is set to `-1`, the operation will never time out.

```cpp
QCoro::Task<QByteArray> QCoroIODevice::readLine(qint64 maxSize = 0)
QCoro::Task<QByteArray> QCoroIODevice::readLine(qint64 maxSize, std::chrono::milliseconds timeout);
```

## `waitForReadyRead()`

Waits for at most `timeout_msecs` milliseconds for data to become available for reading
in the `QIODevice`. Returns `true` when the device becomes ready for reading within the
given timeout. Returns `false` if the operation times out, if the device is not opened
for reading or in any other state in which the device will never become ready for reading.

If the timeout is -1, the operation will never time out.

See documentation for [`QIODevice::waitForReadyRead()`][qtdoc-qiodevice-waitforreadyread] for details.

```cpp
QCoro::Task<bool> QCoroIODevice::waitForReadyRead(int timeout_msecs = 30'000);
QCoro::Task<bool> QCoroIODevice::waitForReadyRead(std::chrono::milliseconds timeout);
```

## `waitForBytesWritten()`

Waits for at most `timeout_msecs` milliseconds for data to be flushed from a buffered
`QIODevice`. Returns `std::optional<qint64>`, which is empty if the operation has timed
out, the device is not opened for writing or is in any other state in which the device
will never be able to write any data. When the data are successfully flushed, returns
number of bytes written.

If the timeout is -1, the operation will never time out.

See documentation for [`QIODevice::waitForBytesWritten()`][qtdoc-qiodevice-waitforbyteswritten] for details.

```cpp
QCoro::Task<std::optional<qint64>> QCoroIODevice::waitForBytesWritten(int timeout_msecs = 30'000);
QCoro::Task<std::optional<qint64>> QCoroIODevice::waitForBytesWritten(std::chrono::milliseconds timeout);
```

## Examples

```cpp
const QByteArray data = co_await qCoro(device).readAll();
```

[qlocalsocket]: ../network/qlocalsocket.md
[qcoro-coro]: ../coro/coro.md
[qtdoc-qiodevice]: https://doc.qt.io/qt-5/qiodevice.html
[qtdoc-qiodevice-read]: https://doc.qt.io/qt-5/qiodevice.html#read
[qtdoc-qiodevice-readyread]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
[qtdoc-qiodevice-readall]: https://doc.qt.io/qt-5/qiodevice.html#readAll
[qtdoc-qiodevice-readline]: https://doc.qt.io/qt-5/qiodevice.html#readLine
[qtdoc-qiodevice-waitforreadyread]: https://doc.qt.io/qt-5/qiodevice.html#waitForReadyRead
[qtdoc-qiodevice-waitforbyteswritten]: https://doc.qt.io/qt-5/qiodevice.html#waitForBytesWritten


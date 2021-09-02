# QIODevice

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

```cpp
Awaitable auto QCoroIODevice::readAll();
```

## `read()`

Waits until there are any data to be read from the device (similar to waiting until the device
emits [`QIODevice::readyRead()`][qtdoc-qiodevice-readyread] signal) and then returns up to
`maxSize` bytes as a `QByteArray`. Doesn't suspend the coroutine if there are already data
available in the `QIODevice` or if the device is not opened for reading.

See documentation for [`QIODevice::read()`][qtdoc-qiodevice-read] for details.

```cpp
Awaitable auto QCoroIODevice::read(qint64 maxSize = 0);
```

## `readLine()`

Repeatedly waits for data to arrive until it encounters a newline character, end-of-data or
until it reads `maxSize` bytes. Returns the resulting data as `QByteArray`.

See documentation for [`QIODevice::readLine()`][qtdoc-qiodevice-readline] for details.

```cpp
Awaitable auto QCoroIODevice::readLine(qint64 maxSize = 0)
```

## Examples

```cpp
const QByteArray data = co_await qCoro(device).readAll();
```

[qlocalsocket]: qlocalsocket.md
[qcoro-coro]: coro.md
[qtdoc-qiodevice]: https://doc.qt.io/qt-5/qiodevice.html
[qtdoc-qiodevice-read]: https://doc.qt.io/qt-5/qiodevice.html#read
[qtdoc-qiodevice-readyread]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
[qtdoc-qiodevice-readall]: https://doc.qt.io/qt-5/qiodevice.html#readAll
[qtdoc-qiodevice-readline]: https://doc.qt.io/qt-5/qiodevice.html#readLine


[![Linux CI](https://github.com/danvratil/qcoro/actions/workflows/build-linux.yml/badge.svg)](https://github.com/danvratil/qcoro/actions/workflows/build-linux.yml)
[![Windows CI](https://github.com/danvratil/qcoro/actions/workflows/build-windows.yml/badge.svg)](https://github.com/danvratil/qcoro/actions/workflows/build-windows.yml)
[![MacOS CI](https://github.com/danvratil/qcoro/actions/workflows/build-macos.yml/badge.svg)](https://github.com/danvratil/qcoro/actions/workflows/build-macos.yml)
[![Docs build](https://github.com/danvratil/qcoro/actions/workflows/update-docs.yml/badge.svg?branch=main)](https://github.com/danvratil/qcoro/actions/workflows/update-docs.yml)
[![Latest release](https://img.shields.io/github/v/release/danvratil/qcoro?label=%F0%9F%93%A6%20Release)](https://github.com/danvratil/qcoro/releases)
![License: MIT](https://img.shields.io/badge/%E2%9A%96%EF%B8%8F%20License-MIT-brightgreen)
![C++20](https://img.shields.io/badge/C%2B%2B-20-%2300599C?logo=cplusplus)
![Supported Compilers](https://img.shields.io/badge/%E2%9A%99%EF%B8%8F%20Compilers-GCC%2C%20clang%2C%20MSVC-informational)

# QCoro - Coroutines for Qt5 and Qt6

The QCoro library provides set of tools to make use of C++20 coroutines with Qt.

Take a look at the example below to see what an amazing thing coroutines are:
```cpp
QNetworkAccessManager networkAccessManager;
// co_await the reply - the coroutine is suspended until the QNetworkReply is finished.
// While the coroutine is suspended, *the Qt event loop runs as usual*.
const QNetworkReply *reply = co_await networkAccessManager.get(url);
// Once the reply is finished, your code resumes here as if nothing amazing has just happened ;-)
const auto data = reply->readAll();
```

It requires a compiler with support for the couroutines TS.

## Documentation

👉 📘 [Documentation](https://qcoro.dvratil.cz/)

## Supported Qt Types

QCoro provides `QCoro::Task<T>` which can be used as a coroutine return type and allows the coroutine
to be awaited by its caller. Additionally, it provides `qCoro()` wrapper function, which wraps an
object of a supported Qt type to a thin, coroutine-friendly wrapper that allows `co_await`ing asynchronous
operations on this type. Finally, it allows to directly `co_await` default asynchronous operations on
certain Qt types. Below is a list of a few supported types, you can find the full list in the
[documentation](https://qcoro.dvratil.cz/reference).

### `QDBusPendingCall`

QCoro can wait for an asynchronous D-Bus call to finish. There's no need to use `QDBusPendingCallWatcher`
with QCoro - just `co_await` the result instead. While co_awaiting, the Qt event loop runs as usual.

```cpp
QDBusInterface remoteServiceInterface{serviceName, objectPath, interface};
const QDBusReply<bool> isReady = co_await remoteServiceInterface.asyncCall(QStringLiteral("isReady"));
```

📘 [Full documentation here](https://qcoro.dvratil.cz/reference/dbus/qdbuspendingcall).

### `QFuture`

QFuture represents a result of an asynchronous task. Normally you have to use `QFutureWatcher` to get
notified when the future is ready. With QCoro, you can just `co_await` it!

```cpp
const QFuture<int> task1 = QtConcurrent::run(....);
const QFuture<int> task2 = QtConcurrent::run(....);

const int a = co_await task1;
const int b = co_await task2;

co_return a + b;
```

📘 [Full documentation here](https://qcoro.dvratil.cz/reference/core/qfuture).

### `QNetworkReply`

Doing network requests with Qt can be tedious - the signal/slot approach breaks the flow
of your code. Chaining requests and error handling quickly become mess and your code is
broken into numerous functions. But not with QCoro, where you can simply `co_await` the
`QNetworkReply` to finish:

```cpp
QNetworkReply qnam;
QNetworkReply *reply = qnam.get(QStringLiteral("https://github.com/danvratil/qcoro"));
const auto contents = co_await reply;
reply->deleteLater();
if (reply->error() != QNetworkReply::NoError) {
    co_return handleError(reply);
}

const auto link = findLinkInReturnedHtmlCode(contents);
reply = qnam.get(link);
const auto data = co_await reply;
reply->deleteLater();
if (reply->error() != QNetworkReply::NoError) {
    co_return handleError(reply);
}
 ...
 ```

📘 [Full documentation here](https://qcoro.dvratil.cz/reference/network/qnetworkreply).

### `QTimer`

Maybe you want to delay executing your code for a second, maybe you want to execute some
code in repeated interval. This becomes super-trivial with `co_await`:

```cpp
QTimer timer;
timer.setInterval(1s);
timer.start();

for (int i = 1; i <= 100; ++i) {
    co_await timer;
    qDebug() << "Waiting for " << i << " seconds...";
}

qDebug() << "Done!";
```

📘 [Full documentation here](https://qcoro.dvratil.cz/reference/core/qtimer).

### `QIODevice`

`QIODevice` is a base-class for many classes in Qt that allow data to be asynchronously
written and read. How do you find out that there are data ready to be read? You could
connect to `QIODevice::readyRead()` singal, or you could use QCoro and `co_await` the object:

```cpp
socket->write("PING");
// Waiting for "pong"
const auto data = co_await socket;
co_return calculateLatency(data);
```

📘 [Full documentation here](https://qcoro.dvratil.cz/reference/core/qiodevice).

### ...and more!

Go check the [full documentation](https://qcoro.dvratil.cz) to learn more.

## .then() continuations

Sometimes it's not possible to use `co_await` to handle result of a coroutine - usually
when interfacing with a 3rd party code that does not support coroutines. In those
scenarios it's possible to chain a continuation callback to the coroutine which will
get invoked asynchronously when the coroutine finishes.

```cpp
void regularFunction() {
    someCoroutineReturningInt().then([](int result) {
        // handle result
    });
}
```

The continuation callback can also be a coroutine and the result of the entire
expression is Task<T> where T is the return type of the continuation. Thanks to
that it's possible to `co_await` the entire chain, or chain multiple `.then()`
continuations.

📘 [Full documentation here](https://qcoro.dvratil.cz/reference/coro/task).

## Generators

Generator is a coroutine that lazily produces multiple values. While there's
nothing Qt-specific, QCoro provides the necessary tools for users to create
custom generators in their Qt applications.

QCoro provides API for both synchronous generators (`QCoro::Generator<T>`)
and asynchronous generators (`QCoro::AsyncGenerator<T>`). Both generators provide
container-like API: `begin()` and `end()` member functions that return iterator-like
objects, which is well-known and established API and makes generators compatible
with existing algorithms.

```cpp
QCoro::Generator<int> fibonacci() {
    quint64 a = 0, b = 0;
    Q_FOREVER {
        co_yield b;
        const auto tmp = b;
        a = b;
        b += tmp;
    }
}

void printFib(quint64 max) {
    for (auto fib : fibonacci()) {
        if (fib > max) {
            break;
        }
        std::cout << fib << std::endl;
    }
}
```

📘 [Full documentation here](https://qcoro.dvratil.cz/reference/coro/generator).

## License

```text
MIT License

Copyright (c) 2022 Daniel Vrátil <dvratil@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

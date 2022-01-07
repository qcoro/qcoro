[![Build](https://github.com/danvratil/qcoro/actions/workflows/build.yml/badge.svg)](https://github.com/danvratil/qcoro/actions/workflows/build.yml)

# QCoro - Coroutines for Qt5 and Qt6

The QCoro library provides set of tools to make use of the C++20 coroutines in connection
with certain asynchronous Qt actions.

Take a look at the example below to see what an amazing thing coroutines are:
```cpp
QNetworkAccessManager networkAccessManager;
// co_await the reply - the coroutine is suspended until the QNetworkReply is finished.
// While the coroutine is suspended, *the Qt event loop runs as usual*.
const QNetworkReply *reply = co_await networkAccessManager.get(url);
// Once the reply is finished, your code resumes here as if nothing amazing has just happened ;-)
const auto data = reply->readAll();
```

This is a rather experimental library that helps me to understand coroutines
in C++.

It requires compiler with support for the couroutines TS.

## Documentation

👉 📘 [Documentation](https://qcoro.dvratil.cz/)

## Supported Qt Types

QCoro supports using the `co_await` keyword with several Qt types:

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


<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

{{ doctable("Core", "QCoroSignal") }}

# Signals

It's possible to `co_await` a single signal emission through a special
overload of the [`qCoro()`][qcoro-coro] function. The below function returns
an awaitable that will suspend the current coroutine until the specified signal
is emitted.

```cpp
Task<SignalResult> qCoro(QObject *obj, QtSignalPtr ptr);
```

The arguments are a pointer to a QObject-derived object and a pointer
to a the object's signal to connect to. Note that if the object is destroyed
while being `co_await`ed, the coroutine will never be resumed.

The returned awaitable produces the signal's arguments. That is, if the
signal has no arguments, the result of the awaitable will be `void`. If
the signal has exactly one argument, then the awaitable produces the value
of the argument. If the signal has more than one arguments, then the result
of the awaitable is a `std::tuple` with all the arguments.

Example:
```
// void signal
co_await qCoro(timer, &QTimer::timeout);

// single-argument signal
const QUrl url = co_await qCoro(reply, &QNetworkReply::redirected);

// multi-argument signal, captured using structured bindings
const auto [exitCode, exitStatus] = co_await qCoro(process, &QProcess::finished);
```

```cpp
Task<std::optional<SignalResult>> qCoro(QObject *obj, QtSignalPtr ptr, std::chrono::milliseconds timeout);
```

An overload that behaves similar to the two-argument overload, but takes an additional
`timeout` argument. If the signal is not emitted within the specified timeout,  the returned
awaitable produces an empty `std::optional`. Otherwise the return type behaves the same way
as the two-argument overload.

## QCoroSignalListener

A helper function that creates an [`AsyncGenerator`][qcoro-asyncgenerator] which yields a value
whenever the signal is emitted.

```cpp
QCoro::AsyncGenerator<SignalArgs> qCoroSignalListener(QObject *obj, QtSignalPtr ptr, std::chrono::milliseconds timeout);
```

The function takes up to three arguments, the `obj` and `ptr` are a QObject-derived object and
a pointer to a signal member function to connect to. The third `timeout` argument is optional.
When the timeout is set, the generator will end if the signal is not emitted within the specified
timeout. When not set, or set to -1, the generator will never terminate on its own, even if
the `obj` QObject is destroyed!

The generator produces all signal emissions, even those that ocur in between the generator being
`co_await`ed. In the example below, even when the `QNetworkReply::downloadProgress()` signal is
emitted while asynchronously processing something in the middle of the `while` loop body, the
emission will not be lost. It will be enqueued, and returned synchronously with the next
`co_await ++it` call.

```cpp
auto listener = qCoroSignalListener(networkReply, &QNetworkReply::downloadProgress);
auto it = co_await listener.begin(); // waits for first emission
while (it != listener.end() && networkReply->isRunning()) {
    const auto [received, total] = *it;
    // do something with results
    //...
    if (received == total || networkReply->isFinished()) {
        break;
    }
    co_await ++it; // waits for next signal emission
}
```

Alternatively, it's possible to use `QCORO_FOREACH` to look over the generator:

```cpp
QCORO_FOREACH(const auto [received, total], qCoroSignalListener(reply, &QNetworkReply::downloadProgress)) {
    // do something with `received` and `total` values
    if (received == total || reply->isFinished()) {
        break;
    }
}
```

!!! warning "Listener doesn't stop listening until destroyed."
    Keep in mind that the `listener` generator will keep listening and collecting the
    signal emissions until it is destroyed, even if you are no longer actively iterating
    over the generator. It is recommended that you destroy the generator as soon as possible
    when you no longer need it.


[qcoro-coro]: ../coro/coro.md
[qcoro-asyncgenerator]: ../coro/asyncgenerator.md

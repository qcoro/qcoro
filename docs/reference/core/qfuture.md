<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QFuture

{{ doctable("Core", "QCoroFuture") }}

[`QFuture`][qdoc-qfuture], which represents an asynchronously executed call, doesn't have any
operation on its own that could be awaited asynchronously, this is usually done through a helper
class called [`QFutureWatcher`][qdoc-qfuturewatcher]. To simplify the API, QCoro allows to directly
`co_await` completion of the running `QFuture` or use of a wrapper class `QCoroFuture`. To wrap
a `QFuture` into a `QCoroFuture`, use [`qCoro()`][qcoro-coro]:

```cpp
template<typename T>
QCoroFuture qCoro(const QFuture<T> &future);
```

It's possible to directly `co_await` a completion of a `QFuture` and obtain a result:

```cpp
const QString result = co_await QtConcurrent::run(...);
```

This is a convenient alternative to using `co_await qCoro(future).result()`.

## `result()`

Asynchronously waits for the `QFuture<T>` to finish and returns the result of the future. If the future
result type is `void`, then returns nothing. If the asynchronous operation has thrown an exception, it
will be rethrown here.

Example:

```cpp
QFuture<QString> future = QtConcurrent::run(...);
const QString result = co_await qCoro(future).result();
```

This is equivalent to using `QFutureWatcher` and then retrieving the result using
[`QFuture::result()`][qdoc-qfuture-result]:

```cpp
QFuture<QString> future = QtConcurrent::run(...);
auto watcher = new QFutureWatcher<QString>();
connect(watcher, &QFutureWatcherBase::finished,
        this, [watcher]() {
            watcher->deleteLater();
            auto result = watcher.result();
            ...
        });
watcher->addFuture(future);
```

## `takeResult()`

Asynchronously waits for the `QFuture<T>` to finish and returns the result of the future by taking
(moving) it from the future object. If the asynchronous operation has thrown an exception, it will
be rethrown here.

This is useful when dealing with move-only types (like `std::unique_ptr`) or when you simply want to
move the result instead of copying it out from the future.

Example:

```cpp
QFuture<std::unique_ptr<Result>> future = QtConcurrent::run(...);
auto result = co_await qCoro(future).takeResult();
```

This is equivalent  to using `QFutureWatcher` and then retrieving the result using
[`QFuture::takeResult()`][qdoc-qfuture-takeResult]:

```cpp
QFuture<std::unique_ptr<Result>> future = QtConcurrent::run(...);
auto watcher = new QFutureWatcher<std::unique_ptr<Result>>();
connect(watcher, &QFutureWatcherBase::finished,
        this, [watcher]() {
            watcher->deleteLater();
            auto result = watcher.future().takeResult();
            ...
        });
watcher->addFuture(future);
```

See documentation for [`QFuture::takeResult()`][qdoc-qfuture-takeResult] for limitations regarding
moving the result out from `QFuture`.

!!! info "This method is only available in Qt 6."

## `waitForFinished()`

This is equivalent to using the [`result()`](#result) method.

## Example

```cpp
{% include "../../examples/qfuture.cpp" %}
```

[qdoc-qfuture]: https://doc.qt.io/qt-5/qfuture.html
[qdoc-qfuturewatcher]: https://doc.qt.io/qt-5/qfuturewatcher.html
[qdoc-qfuture-result]: https://doc.qt.io/qt-6/qfuture.html#result
[qdoc-qfuture-takeResult]: https://doc.qt.io/qt-6/qfuture.html#takeResult
[qcoro-coro]: ../coro/coro.md

<!--
SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QCoro::LazyTask

{{ doctable("Coro", "QCoroLazyTask", None, [], "0.11") }}

```cpp
template<typename T> class QCoro::LazyTask
```

`QCoro::LazyTask<T>` represents, as the name suggests, a lazily evaluated coroutine. A lazily
evaluated coroutine is suspended immediately when called and will not execute its body until
the returned `QCoro::LazyTask<T>` is `co_await`ed.

This is in contrast to `QCoro::Task<T>`, which is an eager coroutine, meaning that its body
is executed immediately when invoked and is only suspended when it first `co_await`s another
coroutine.

!!! warning "Don't use LazyTask<T> as a Qt slot"

    Do not use `LazyTask<T>` as a return type of Qt slots, or any other coroutine that can be
    invoked by the  Qt event loop. The Qt event loop is not aware of coroutines and will never
    `co_await` the returned `LazyTask<T>`. Therefore, the coroutine's body would never be
    executed!

    This is the main reason why the "default" `QCoro::Task<T>` coroutines are eager - they
    don't need to be `co_await`ed in order to be executed, which makes them compatible with 
    the Qt event loop.

    If you need to have a lazy coroutine that is also invokable from the Qt event loop, use an
    eager wrapper coroutine to pass to the event loop:

    ```cpp
    QCoro::LazyTask<> myLazyCoroutine();

    Q_INVOKABLE QCoro::Task<> myLazyCoroutineWrapper() {
        co_await myLazyCoroutine();
    }
    ```

    The eager wrapper is always immediately executed, and since it will immediately start
    `co_await`ing on the lazy coroutine, the lazy coroutine will effectively get executed
    immediately.

## `then()` continuation

It is possible to chain a continuation to the coroutine by using `.then()`. It is possible to
use both lazy and eager continuations and even non-coroutine continuations:

```cpp
auto task = myLazyTask().then([]() -> QCoro::Task<> { ... });        // #1
auto task = myLazyTask().then([]() -> QCoro::LazyTask<> { ... });    // #2
auto task = myLazyTask().then([]() { return 42; })                   // #3
```

In case #1, the `myLazyTask()` will be evaluated eagerly becaues of the `.then()` continuation being
eager (basically equivalent to the eager wrapper mentioned in the warning note at the top).

In case #2 and #3, the the entire chain will be evaluated lazily - that is, both the `myLazyTask()` and the
`.then()` continuation will be evaluated only once `task` is `co_await`ed.

## Blocking wait

It's possible to use `QCoro::waitFor()` to synchronously wait for completion of a lazy coroutine.
Check the documentation in [`QCoro::Task<T>`](task.md#Blocking-wait) for details.

## Interfacing with synchronous functions

See [`QCoro::connect()  documentation for `QCoro::Task<T>`](task.md#interfacing-with-synchronous-functions)
for details.

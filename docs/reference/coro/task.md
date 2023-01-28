<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QCoro::Task

{{ doctable("Coro", "QCoroTask") }}

```cpp
template<typename T> class QCoro::Task
```

Any coroutine that wants to `co_await` one of the types supported by the QCoro library must have
return type `QCoro::Task<T>`, where `T` is the type of the "regular" coroutine return value.

There's no need by the user to interact with or construct `QCoro::Task` manually, the object is
constructed automatically by the compiler before the user code is executed. To return a value
from a coroutine, use `co_return`, which will store the result in the `Task` object and leave
the coroutine.

```cpp
QCoro::Task<QString> getUserName(UserID userId) {
    ...

    // Obtain a QString by co_awaiting another coroutine
    const QString result = co_await fetchUserNameFromDb(userId);

    ...

    // Return the QString from the coroutine as you would from a regular function,
    // just use `co_return` instead of `return` keyword.
    co_return result;
}
```

To obtain the result of a coroutine that returns `QCoro::Task<T>`, the result must be `co_await`ed.
When the coroutine `co_return`s a result, the result is stored in the `Task` object and the `co_await`ing
coroutine is resumed. The result is obtained from the returned `Task` object and returned as a result
of the `co_await` call.

```cpp
QCoro::Task<void> getUserDetails(UserID userId) {
    ...

    const QString name = co_await getUserName(userId);

    ...
}
```

!!! info "Exception Propagation"
    When coroutines throws an unhandled exception, the exception is stored in the `Task` object and
    is re-thrown from the `co_await` call in the awaiting coroutine.

## `then()` continuation

!!! note "This feature is available since QCoro 0.5.0"

Sometimes it's not possible to `co_await` a coroutine, for example when calling a coroutine from a
reimplementation of a virtual function from a 3rd party library, where we cannot change the signature
of that function to be a coroutine (e.g. a reimplementation of `QAbstractItemModel::data()`).

Even in this case, we want to process the result of the coroutine asynchronously, though. For such
cases, `Task<T>` provides a `then()` member function that allows the caller to provide a custom
continuation callback to be invoked when the coroutine finishes.

---

```cpp
template<typename ThenCallback>
requires (std::invocable<ThenCallback> || (!std::is_void<T> && std::invocable<ThenCallback, T>))
Task<R> Task<T>::then(ThenCallback callback);
```

The `Task<T>::then()` member function has two arguments. The first argument is the continuation
that is called when the coroutine finishes. The second argument is optional - it is a callable
that gets invoked instead of the continuation when the coroutine throws an exception.

The continuation callback must be a callable that accepts either zero arguments (effectively
discardin the result of the coroutine), or exactly one argument of type `T` or type implicitly
constructible from `T`.

If the return type of the `ThenCallback` is `void`, then the return type of the `then()` functon is
`Task<void>`. If the return type of the `ThenCallback` is `R` or `Task<R>`, the return type of the
`then()` function is `Task<R>`. This means that the `ThenCallback` can be a coroutine as well. Thanks
to the return type always being of type `Task<R>`, it is possible to chain multiple `.then()` calls,
or `co_await` the result of the entire chain.

If the coroutine throws an exception, the exception is re-thrown when the result of the entire
continuation is `co_await`ed. If the result of the continuation is not `co_await`ed, the exception
is silently ignored.

If an exception is thrown from the `ThenCallback`, then the exception is either propagated to the nex
chained `then()` continuation or re-thrown if directly `co_await`ed. If the result is not `co_await`ed
and no futher `then()` continuation is chained after the one that has thrown, then the exception is
silently ignored.

---

```cpp
template<typename ThenCallback, typename ErrorCallback>
requires (((std::is_void_t<T> && std::invocable<ThenCallback>) || std::invocable<ThenCallback, T>)
            && std::invocable<ErrorCallback, const std::exception &>)
Task<R> Task<T>::then(ThenCallback thenCallback, ErrorCallback errorCallback);
```

An overload of the `then()` member function which takes an additional callback to be invoked when
an exception is thrown from the coroutine. The `ErrorCallback` must be a callable that takes exactly
one argument, which is `const std::exception &`, holding reference to the exception thrown. An exception
thrown from the `ErrorCallback` will be re-thrown if the entire continuation is `co_await`ed. If another
`.then()` continuation is chained after the current continuation and has an `ErrorCallback`, then the
`ErrorCallback` will be invoked. Otherwise, the exception is silently ignored.

If an exception is thrown by the non-void coroutine and is handled by the `ErrorCallback`, then if the
resulting continuation is `co_await`ed, the result will be a default-constructed instance of type `R`
(since the `ThenCallback` was unable to provide a proper instance of type `R`). If `R` is not default-
constructible, the program will not compile. Thus, if returning a non-default-constructible type from
a coroutine that may throw an exception, we recommend to wrap the type in `std::optional`.

Examples:

```cpp
QString User::name() {
    if (mName.isNull()) {
        mApi.fetchUserName().then(
            [this](const QString &name) {
                mName = name;
                Q_EMIT nameChanged();
            }, [](const std::exception &e) {
                mName = QStringLiteral("Failed to fetch name: %1").arg(e.what());
                Q_EMIT nameChanged();
            });
        return QStringLiteral("Loading...");
    } else {
        return mName;
    }
}
```

## Blocking wait

Sometimes it's necessary to wait for a coroutine in a blocking manner - this is especially useful
in tests where possibly no event loop is running. QCoro has `QCoro::waitFor()` function
which takes `QCoro::Task<T>` (that is, result of calling any QCoro-based coroutine) and blocks
until the coroutine finishes. If the coroutine has a non-void return value, the value is returned
from `waitFor().`

Since QCoro 0.8.0 it is possible to use `QCoro::waitFor()` with any awaitable type, not just `QCoro::Task<T>`.

```cpp
QCoro::Task<int> computeAnswer() {
    co_await QCoro::sleepFor(std::chrono::years{7'500'00});
    co_return 42;
}

void nonCoroutineFunction() {
    // The following line will block as if computeAnswer were not a coroutine. It will internally run a
    // a QEventLoop, so other events are still processed.
    const int answer = QCoro::waitFor(computeAnswer());
    std::cout << "The answer is: " << answer << std::endl;
}
```

!!! info "Event loops"
    The implementation internally uses a `QEventLoop` to wait for the coroutine to be completed.
    This means that a `QCoreApplication` instance must exist, although it does not need to be
    executed. Usual warnings about using a nested event loop apply here as well.

## Interfacing with synchronous functions

!!! note "This feature is available since QCoro 0.7.0"

Sometimes you need to interface with code that is not coroutine-aware, for example when building list models.

If you'd use the normal `.then()` function, and the coroutine would finish after the model is deleted, the program would crash.

The `QCoro::connect` function is similar to `QObject::connect`,
but for `QCoro::Task`s.
Just like `QObject::connect`, it only calls its callback if the context object still exists.

This is an example for a function that fetches data and updates a model:
```cpp
void updateModel() {
    auto task = someCoroutine();
    QCoro::connect(std::move(task), this, [this](auto &&result) {
        beginResetModel();
        m_entries = std::move(result);
        endResetModel();
    });
}
```

If the model is deleted before the coroutine finishes, the connected lambda will not be called.

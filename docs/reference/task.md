# QCoro::Task

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


## Blocking wait

Sometimes it's necessary to wait for a coroutine in a blocking manner - this is especially useful
in tests where possibly no event loop is running. QCoro has `QCoro::blockingWait()` function
which takes `QCoro::Task<T>` (that is, result of calling any QCoro-based coroutine) and blocks
until the coroutine finishes. If the coroutine has a non-void return value, the value is returned
from `blockingWait().`

```cpp
QCoro::Task<int> computeAnswer() {
    std::this_thread::sleep_for(std::chrono::year{7'500'000});
    co_return 42;
}

void nonCoroutineFunction() {
    // The following line will block as if computeAnswer were not a coroutine.
    const int answer = QCoro::blockingWait(computeAnswer());
    std::cout << "The answer is: " << answer << std::endl;
}
```

!!! info "Event loops"
    The implementation internally uses a `QEventLoop` to wait for the coroutine to be completed.
    This means that a `QCoreApplication` instance must exist, although it does not need to be
    executed. Usual warnings about using a nested event loop apply here as well.

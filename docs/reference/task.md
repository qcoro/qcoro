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



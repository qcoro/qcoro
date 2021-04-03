# QFuture

```cpp
const QString result = co_await QtConcurrent::run(taskReturningQString);
```

The QCoro frameworks allows `co_await`ing on [QFuture][qdoc-qfuture] objects,
which represent an asynchronously executed call. The co-awaiting coroutine is suspended until
the `QFuture` is finished. If the `QFuture` object is already finished, the coroutine
will not be suspended.

To be able to use `co_await` with `QFuture`, include `qcoro/future.h` in your implementation.

```cpp
#include <qcoro/future.h>

QCoro::Task<> runTask() {
    // Starts a concurrent task and co_awaits on the returned QFuture. While the task is
    // running, the coroutine is suspended.
    const QString value = co_await QtConcurrent::run([]() {
        QString result;
        ...
        // do some long-running computation
        ...
        return result;
    });
    // When the future has finished, the coroutine is resumed and the result of the
    // QFuture is returned and stored in `value`.

    // ... now do something with the value
}
```

[qdoc-qfuture]: https://doc.qt.io/qt-5/qfuture.html

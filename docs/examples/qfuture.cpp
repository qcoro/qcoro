#include <QCoroFuture>

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

# QCoro::coro()

```cpp
QCoroType QCoro::coro(QtClass *);
QCoroType QCoro::coro(QtClass &);
```

This function is overloaded for all Qt types supported by this library. It accepts either
a pointer or a reference to a Qt type, and returns a QCoro type that wraps the Qt type and
provides coroutine-friendly API for the type.

Some objects have only a single asynchronous event, so it makes sense to make them
directly `co_await`able. An example is `QTimer`, where only one think can be `co_await`ed -
the timer timeout. Thus with QCoro, it's possible to simply do this:

```cpp
QTimer timer;
...
co_await timer;
```

However, some Qt classes have multiple asynchronous operations that the user may want to `co_await`.
For such types, simply `co_await`ing the class instance doesn't make sense since it's not clear
what operation is being `co_await`ed. For those types, QCoro provides `QCoro::coro()` function
which returns a wrapper that provides coroutine-friendly versions of the asynchronous methods
for the given type.

Let's take QProcess as an example: one may want to `co_await` for the program to start or finish.
Therefor, the type must be wrapped into `QCoro::coro()` like this:

```cpp
QProcess process;
// Wait for the process to be started
co_await QCoro::coro(process).start(...);
// The process is running now
...
...
// Wait for it to finish
co_await QCoro::coro(process).finished();
// The process is no longer running
...
```

`QCoro::coro()` is simply overloaded for all the Qt types currently supported by the QCoro library.
The function returns a wrapper object (e.g. `QCoro::detail::QProcessWrapper`) which copies the
QProcess API. It doesn't copy the entire API, only the bits that we want to make `co_await`able.
When you call one of those metods (e.g. `QProcessWrapper::start()`), it returns an awaitable
type that calls `QProcess::start()`, suspends the coroutine and resumes it again when the
wrapped `QProcess` object emits the `started()` signal.

Normally you don't need to concern yourself with anything inside the `QCoro::detail` namespace,
it's mentioned in the previous paragraph simply to explain how the wrapper works.

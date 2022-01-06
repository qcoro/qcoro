# Coro module

The Coro module contains the fundamental coroutine functionality - the
coroutine return type [QCoro::Task<T>][qcoro-task]. Another useful bit
of the Coro module is the [qCoro()][qcoro-coro] wrapper function that
wraps native Qt types into a coroutine-friendly versions supported by
QCoro (check the Core, Network and DBus modules of QCoro to see which
Qt types are currently supported by QCoro).

If you don't want to use any of the Qt types supported by QCoro in your
code, but you still want to use C++ coroutines with QCoro, you can simply
just link against `QCoro::Coro` target in your CMakeLists.txt. This will
give you all you need to start implementing custom coroutine-native types
with Qt and QCoro.

[qcoro-task]: task.md
[qcoro-coro]: coro.md

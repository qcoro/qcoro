<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Coro module

The Coro module contains the fundamental coroutine types - the
[QCoro::Task&lt;T>][qcoro-task] for asynchronous coroutines,
[QCoro::Generator&lt;T>][qcoro-generator] for synchronous generators and
[QCoro::AsyncGenerator&lt;T>][qcoro-asyncgenerator] for asynchronous generators.
Another useful bit of the Coro module is the [qCoro()][qcoro-coro] wrapper
function that wraps native Qt types into a coroutine-friendly versions supported by
QCoro (check the [Core][qcoro-core], [Network][qcoro-network] and
[DBus][qcoro-dbus] modules of QCoro to see which
Qt types are currently supported by QCoro).

If you don't want to use any of the Qt types supported by QCoro in your
code, but you still want to use C++ coroutines with QCoro, you can simply
just link against `QCoro::Coro` target in your CMakeLists.txt. This will
give you all you need to start implementing custom coroutine-native types
with Qt and QCoro.

[qcoro-task]: task.md
[qcoro-coro]: coro.md
[qcoro-generator]: generator.md
[qcoro-asyncgenerator]: asyncgenerator.md
[qcoro-core]: ../core/index.md
[qcoro-network]: ../network/index.md
[qcoro-dbus]: ../dbus/index.md

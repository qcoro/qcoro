<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QProcess

{{ doctable("Core", "QCoroProcess", ("core/qiodevice", "QCoroIODevice")) }}

[`QProcess`][qtdoc-qprocess] normally has two features to wait for asynchronously: the process to start
and to finish. Since `QProcess` itself doesn't provide the ability to `co_await` those operations,
QCoro provides a wrapper class `QCoroProcess`. To wrap a `QProcess` object into the `QCoroProcess`
wrapper, use [`qCoro()`][qcoro-coro]:

```cpp
QCoroProcess qCoro(QProcess &);
QCoroProcess qCoro(QProcess *);
```

Same as `QProcess` is a subclass of `QIODevice`, `QCoroProcess` subclasses [`QCoroIODevice`][qcoro-qcoroiodevice],
so it also provides the awaitable interface for selected QIODevice functions.
See [`QCoroIODevice`][qcoro-qcoroiodevice] documentation for details.

## `waitForStarted()`

Waits for the process to be started or until it times out. Returns `bool` indicating
whether the process has started successfuly (`true`) or timed out (`false`).

See documentation for [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted] for details.

```cpp
QCoro::Task<bool> QCoroProcess::waitForStarted(int timeout = 30'000);
QCoro::Task<bool> QCoroProcess::waitForStarted(std::chrono::milliseconds timeout);
```

## `waitForFinished()`

Waits for the process to finish or until it times out. Returns `bool` indicating
whether the process has finished successfuly (`true`) or timed out (`false`).

See documentation for [`QProcess::waitForFinished()`][qtdoc-qprocess-waitForFinished] for details.

```cpp
QCoro::Task<bool> QCoroProcess::waitForFinishedint timeout = 30'000);
QCoro::Task<bool> QCoroProcess::waitForFinished(std::chrono::milliseconds timeout);

```

## `start()`

QCoroProcess provides an additional method called `start()` which is equivalent to calling
`QProcess::start()` followed by `QCoroProcess::waitForStarted()`. This operation is `co_awaitable`
as well.

See the documentation for [`QProcess::start()`][qtdoc-qprocess-start] and
[`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted] for details.o

Returns `true` when the process has successfully started, `false` otherwise.

```cpp
QCoro::Task<bool> QCoroProcess::start(QIODevice::OpenMode openMode = QIODevice::ReadOnly,
                                      std::chrono::milliseconds timeout = std::chrono::seconds(30));
QCoro::Task<bool> QCoroProcess::start(const QString &program, const QStringList &arguments,
                                      QIODevice::OpenMode openMode = QIODevice::ReadOnly,
                                      std::chrono::milliseconds timeout = std::chrono::seconds(30));
```

## Examples

```cpp
{% include "../../examples/qprocess.cpp" %}
```


[qtdoc-qprocess]: https://doc.qt.io/qt-5/qprocess.html
[qtdoc-qprocess-start]: https://doc.qt.io/qt-5/qprocess.html#start
[qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
[qtdoc-qprocess-waitForFiished]: https://doc.qt.io/qt-5/qprocess.html#waitForFinished
[qcoro-coro]: ../coro/coro.md
[qcoro-qcoroiodevice]: qiodevice.md

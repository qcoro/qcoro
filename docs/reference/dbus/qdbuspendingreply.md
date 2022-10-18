<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QDBusPendingReply

 {{ doctable("DBus", "QCoroDBusPendingReply") }}

[`QDBusPendingReply`][qdoc-qdbuspendingreply] on its own doesn't have any operation that could
be awaited asynchronously, this is usually done through a helper class called
[`QDBusPendingCallWatcher`][qdoc-qdbuspendingcallwatcher]. To simplify the API, QCoro allows to
directly `co_await` completion of the pending reply or use a wrapper class `QCoroDBusPendingReply`.
To wrap a `QDBusPendingReply` into a `QCoroDBusPendingReply`, use [`qCoro()`][qcoro-coro]:

```cpp
template<typename ... Args>
QCoroDBusPendingCall qCoro(const QDBusPendingReply<Args ...> &);
```

!!! info "`QDBusPendingReply` in Qt5 vs Qt6"
    `QDBusPendingReply` in Qt6 is a variadic template, meaning that it can take any amount of template arguments.
    In Qt5, however, `QDBusPendingReply` is a template class that accepts only up to 8 paremeters. In QCoro the
    `QCoroDBusPendingReply` wrapper is implemented as a variadic template for compatibility with Qt6, but when
    building against Qt5, the number of template parameters is artificially limited to 8 to mirror the limitation
    of Qt5 `QDBusPendingReply` limitation.

To await completion of the pending call without the `qCoro` wrapper, just use the pending call
in a `co_await` expression. The behavior is identical to awaiting on result of
`QCoroDBusPendingReply::waitForFinished()`.

```cpp
QDBusPendingReply<...> reply = interface.asyncCall(...);
co_await reply;
// Now the reply is finished and the result can be retrieved.
```

## `waitForFinished()`

{% include-markdown "../../../qcoro/dbus/qcorodbuspendingreply.h"
    dedent=true
    rewrite-relative-urls=false
    start="<!-- doc-waitForFinished-start -->"
    end="<!-- doc-waitForFinished-end -->" %}

## Example

```cpp
{% include "../../examples/qdbus.cpp" %}
```

[qdoc-qdbuspendingcall]: https://doc.qt.io/qt-5/qdbuspendingcall.html
[qdoc-qdbuspendingreply]: https://doc.qt.io/qt-5/qdbuspendingreply.html
[qdoc-qdbuspendingcallwatcher]: https://doc.qt.io/qt-5/qdbuspendingcallwatcher.html
[qdoc-qdbuspendingcallwatcher-finished]: https://doc.qt.io/qt-5/qdbuspendingcallwatcher.html#finished
[qdoc-qdbusabstractinterface-asyncCall]: https://doc.qt.io/qt-5/qdbusabstractinterface.html#asyncCall-1
[qcoro-coro]: ../coro/coro.md

<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QDBusPendingCall

{{ doctable("DBus", "QCoroDBusPendingCall") }}


[`QDBusPendingCall`][qdoc-qdbuspendingcall] on its own doesn't have any operation that could
be awaited asynchronously, this is usually done through a helper class called
[`QDBusPendingCallWatcher`][qdoc-qdbuspendingcallwatcher]. To simplify the API, QCoro allows to
directly `co_await` completion of the pending call or use a wrapper class `QCoroDBusPendingCall`.
To wrap a `QDBusPendingCall` into a `QCoroDBusPendingCall`, use [`qCoro()`][qcoro-coro]:

```cpp
QCoroDBusPendingCall qCoro(const QDBusPendingCall &);
```

To await completion of the pending call without the `qCoro` wrapper, just use the pending call
in a `co_await` expression. The behavior is identical to awaiting on result of
`QCoroDBusPendingCall::waitForFinished()`.

```cpp
QDBusPendingCall call = interface.asyncCall(...);
const QDBusReply<...> reply = co_await pendigCall;
```

!!! info "`QDBusPendingCall` vs. `QDBusPendingReply`"
    As the Qt documentation for [`QDBusPendingCall`][qdoc-qdbuspendingcall] says, you are more likely to use
    [`QDBusPendingReply`][qdoc-qdbuspendingreply] in application code than `QDBusPendingCall`. QCoro has explicit
    support for `QDBusPendingCall` to allow using functions that return `QDBusPendingCall` directly in `co_await`
    expressions without the programmer having to first convert it to `QDBusPendingReply`. `QDBusPendingReply` can
    be constructed from a `QDBusMessage`, which is a result of awaiting `QDBusPendingCall`, therefore it's possible
    to perform both the conversion and awaiting in a single line of code:
    ```cpp
    QDBusPendingReply<...> reply = co_await iface.asyncCall(...);
    ```
    Note that [`QDBusAbstractInterface::asyncCall`][qdoc-qdbusabstractinterface-asyncCall] returns
    a `QDBusPendingCall`.

## `waitForFinished()`

{% include-markdown "../../../qcoro/dbus/qcorodbuspendingcall.h"
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

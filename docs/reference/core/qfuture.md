<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QFuture

{{ doctable("Core", "QCoroFuture") }}

[`QFuture`][qdoc-qfuture], which represents an asynchronously executed call, doesn't have any
operation on its own that could be awaited asynchronously, this is usually done through a helper
class called [`QFutureWatcher`][qdoc-qfuturewatcher]. To simplify the API, QCoro allows to directly
`co_await` completion of the running `QFuture` or use a wrapper class `QCoroFuture`. To wrap
a `QFuture` into a `QCoroFuture`, use [`qCoro()`][qcoro-coro]:

```cpp
template<typename T>
QCoroFuture qCoro(const QFuture<T> &future);
```

## `waitForFinished()`

{% include-markdown "../../../qcoro/core/qcorofuture.h"
    dedent=true
    rewrite-relative-urls=false
    start="<!-- doc-waitForFinished-start -->"
    end="<!-- doc-waitForFinished-end -->" %}

## Example

```cpp
{% include "../../examples/qfuture.cpp" %}
```

[qdoc-qfuture]: https://doc.qt.io/qt-5/qfuture.html
[qdoc-qfuturewatcher]: https://doc.qt.io/qt-5/qfuturewatcher.html
[qcoro-coro]: ../coro/coro.md

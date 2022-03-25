<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QNetworkReply

{{ doctable("Network", "QCoroNetworkReply", ("network/qiodevice", "QCoroIODevice")) }}

[`QNetworkReply`][qdoc-qnetworkreply] has two asynchronous aspects: one is waiting for the
reply to finish, and one for reading the response data as they arrive. QCoro supports both.
`QNetworkReply` is a subclass of [`QIODevice`][qdoc-qiodevice], so you can leverage all the
features of [`QCoroIODevice`][qcoro-iodevice] to asynchronously read data from the underlying
`QIODevice` using coroutines.

To wait for the reply to finish, one can simply `co_await` the reply object:

```cpp
QNetworkAccessManager nam;
auto *reply = co_await nam.get(request);
```

The QCoro frameworks allows `co_await`ing on [QNetworkReply][qdoc-qnetworkreply] objects. The
co-awaiting coroutine is suspended, until [`QNetworkReply::finished()`][qdoc-qnetworkreply-finished]
signal is emitted.

To make it work, include `QCoroNetworkReply` in your implementation.

```cpp
{% include "../../examples/qnetworkreply.cpp" %}
```

[qdoc-qnetworkreply]: https://doc.qt.io/qt-5/qnetworkreply.html
[qdoc-qnetworkreply-finished]: https://doc.qt.io/qt-5/qnetworkreply.html#finished
[qdoc-qiodevice]: https://doc.qt.io/qt-5/qiodevice.html
[qcoro-iodevice]: ../core/qiodevice.md


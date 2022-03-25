<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Qt vs. co_await

One of the best examples where coroutines simplify your code is when dealing with asynchronous
operations, like network operations.

Let's see how a simple HTTP request would be handled in Qt using the signals/slots mechanism:

```cpp
void MyClass::fetchData() {
    auto *nam = new QNetworkAccessManager(this);
    auto *reply = nam->get(QUrl{QStringLiteral("https://.../api/fetch")});
    QObject::connect(reply, &QNetworkReply::finished,
                     [reply, nam]() {
                         const auto data = reply->readAll();
                         doSomethingWithData(data);
                         reply->deleteLater();
                         nam->deleteLater();
                     });
}
```

Now let's see how the code looks like if we use coroutines:

```cpp
QCoro::Task<> MyClass::fetchData() {
    QNetworkReply nam;
    auto *reply = co_await nam.get(QUrl{QStringLiteral("https://.../api/fetch")});
    const auto data = reply->readAll();
    reply->deleteLater();
    doSomethingWithData(data);
}
```

The magic here is the `co_await` keyword which has turned our method `fetchData()`
into a coroutine and suspended its execution while the network request was running.
When the request finishes, the coroutine is resumed from where it was suspended and
continues.

And the best part? While the coroutine is suspended, the Qt event loop runs as usual!


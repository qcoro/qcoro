# QNetworkReply

```cpp
QNetworkAccessManager nam;
auto *reply = co_await nam.get(request);
```

The QCoro frameworks allows `co_await`ing on [QNetworkReply][qdoc-qnetworkreply] objects. The
co-awaiting coroutine is suspended, until [`QNetworkReply::finished()`][qdoc-qnetworkreply-finished]
signal is emitted.

To make it work, include `qcoro/network.h` in your implementation.

```cpp
#include <qcoro/network.h>

QCoro::Task<> MyClass::fetchData() {
    // Creates QNetworkAccessManager on stack
    QNetworkAccessManager nam;
    // Calls QNetworkAccessManager::get() and co_awaits on the returned QNetworkReply*
    // until it finishes. The current coroutine is suspended until that.
    auto *reply = co_await nam.get(QUrl{QStringLiteral("https://.../api/fetch")});
    // When the reply finishes, the coroutine is resumed and we can access the reply content.
    const auto data = reply->readAll();
    // Raise your hand if you never forgot to delete a QNetworkReply...
    delete reply;
    doSomethingWithData(data);
    // Extra bonus: the QNetworkAccessManager is destroyed automatically, since it's on stack.
}
```

[qdoc-qnetworkreply]: https://doc.qt.io/qt-5/qnetworkreply.html
[qdoc-qnetworkreply-finished]: https://doc.qt.io/qt-5/qnetworkreply.html#finished

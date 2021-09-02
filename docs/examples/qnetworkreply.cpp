#include <QCoroNetworkReply>

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


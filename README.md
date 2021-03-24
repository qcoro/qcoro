# QCoro - Coroutines for Qt

The QCoro library provides set of tools to make use of the C++20 coroutines
in connection with certain asynchronous Qt actions.

This is a rather experimental library that helps me to understand coroutines
in C++.

It requires compiler with support for the couroutines TS.

# Qt Signal/Slots vs. co_await

One of the simplest examples where coroutines simplify your code is when
dealing with asynchronous operations, like DBus calls.

Let's see how a simple pair of dependent DBus calls would be implemented in Qt using
the signals/slots mechanism:
```
void goToNextSong() {
    QDBusInterface spotifyPlayer("org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2",
                                 "org.mpris.MediaPlayer2.Player");
    auto *watcher = new QDBusPendingCallWatcher{spotifyPlayer.callAsync("CanGoNext")};
    connect(watcher, &QDBusPendingCallWatcher::finished,
            [spotifyPlayer](auto *watcher) {
                watcher->deleteLater();
                QDBusReply<bool> reply{*watcher};
                if (static_cast<bool>(reply)) {
                    auto *watcher2 = new QDBusPendingCallWatcher{spotifyPlayer.asyncCall("Next")};
                    connect(watcher, &QDBusPendingCallWatcher::finished,
                            [](auto *watcher) {
                                watcher->deleteLater();
                                QDBusReply<void> reply{*watcher};
                                if (reply.error().isValid()) {
                                    qWarning() << "DBus call failed:" << reply.error().message();
                                }
                            });
                } else {
                    qDebug() << "Last song, can't go to next song.";
                }
            });
}
```

All that the code above does is that it calls "CanGoNext" and if result is `true`,
it calls "Next".

Now let's see how the code looks like if we use coroutines:

```
QCoro::Task<> goToNextSong() {
    QDBusInterface spotifyPlayer("org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2",
                                 "org.mpris.MediaPlayer2.Player");
    auto reply = co_await spotifyPlayer.asyncCall("CanGoNext");
    if (!static_cast<bool>(reply)) {
        qDebug() << "Last song, can't go to next song.";
        return;
    }

    auto reply = co_await spotifyPlayer.asyncCall("Next");
    if (reply.error().isValid()) {
        qWarning() << "DBus call failed:" << reply.error().message();
    }
}
```

The magic here is the `co_await` keyword which has turned our function `goToNextSong()`
into a coroutine and suspended its execution while it waits for the QDBusPendingCall
returned from `asyncCall()` to finish. When it finishes, the coroutine is resumed from
where it was suspended and continues. 

And the best part? While the coroutine is suspended, the Qt event loop runs as usual!


# Thank You

This library is inspired by Lewis Bakers' cppcoro library, which also served as a guide to implementing
the coroutine machinery, alongside his great series on C++ coroutines:
 * [Coroutine Theory](https://lewissbaker.github.io/2017/09/25/coroutine-theory)
 * [Understanding Operator co_await](https://lewissbaker.github.io/2017/11/17/understanding-operator-co-await)
 * [Understanding the promise type](https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type)
 * [Understanding Symmetric Transfer](https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer)


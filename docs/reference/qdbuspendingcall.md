# QDBusPendingCall

```
QDBusPendingCall callReturningQString = ...
const QDBusReply<QString> reply = co_await callReturningQString;
```

The QCoro frameworks allows `co_await`ing on [QDBusPendingCall][qdoc-qdbuspendingcall] objects,
which represent an asynchronous D-Bus call. The co-awaiting coroutine is suspended until the
D-Bus call finishes. If the call has already finished, to coroutine will not be suspended.

To make it work, include `qcoro/dbus.h` in your implementation.

```cpp
#include <qcoro/dbus.h>

QCoro::Task<QString> PlayerControl::nextSong() {
    // Create a regular QDBusInterface representing the Spotify MPRIS interface
    QDBusInterface spotifyPlayer{QStringLiteral("org.mpris.MediaPlayer2.spotify"),
                                 QStringLiteral("/org/mpris/MediaPlayer2"),
                                 QStringLiteral("org.mpris.MediaPlayer2.Player")};
    // Call CanGoNext DBus method and co_await reply. During that the current coroutine is suspended.
    const QDBusReply<bool> canGoNext = co_await spotifyPlayer.asyncCall(QStringLiteral("CanGoNext"));
    // Response has arrived and coroutine is resumed. If the player can go to the next song,
    // do another async call to do so.
    if (static_cast<bool>(canGoNext)) {
        // co_await the call to finish, but throw away the result
        co_await spotifyPlayer.asyncCall(QStringLiteral("Next"));
    }

    // Finally, another async call to retrieve new track metadata. Once again, the coroutine
    // is suspended while we wait for the result.
    const QDBusReply<QVariantMap> metadata = co_await spotifyPlayer.asyncCall(QStringLiteral("Metadata"));
    // Since this function uses co_await, it is in fact a coroutine, so it must use co_return in order
    // to return our result. By definition, the result of this function can be co_awaited by the caller.
    co_return static_cast<const QVariantMap &>(metadata)[QStringLiteral("xesam:title")].toString();
}
```

[qdoc-qdbuspendingcall]: https://doc.qt.io/qt-5/qdbuspendingcall.html


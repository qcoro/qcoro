# QCoro - Coroutines for Qt

The QCoro library provides set of tools to make use of the C++20 coroutines in connection
with certain asynchronous Qt actions.

Take a look at the example below to see what an amazing thing coroutines are:
```cpp
QNetworkAccessManager networkAccessManager;
// co_await the reply - the coroutine is suspended until the QNetworkReply is finished.
// While the coroutine is suspended, *the Qt event loop runs as usual*.
const QNetworkReply *reply = co_await networkAccessManager.get(url);
// Once the reply is finished, your code resumes here as if nothing amazing has just happened ;-)
const auto data = reply->readAll();
```

This is a rather experimental library that helps me to understand coroutines
in C++.

It requires compiler with support for the couroutines TS.

 * [Qt Signals/Slots vs co_await](#qt-signalslots-vs-co_await)
 * [`co_await` explained](#co_await-explained)
 * [Features](#features)
   * [`co_await QNetworkReply`](#co_await-qnetworkreply)
   * [`co_await QDBusPendingCall`](#co_await-qdbuspendingcall)
   * [`co_await QFuture<T>`](#co_await-qfuturet)
 * [`QCoro::Task`](#qcorotask)
 * [Using QCoro](#using-qcoro)
 * [Compiler Support](#compiler-support)
 * [Thank You](#thank-you)

## Qt Signal/Slots vs. co_await

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

## `co_await` Explained

The following paragraphs try to explain what is a coroutine and what `co_await` does in some
simple way. I don't guarantee that any of this is factically correct. For more gritty (and
correct) details, refer to the articles linked at the bottom of this document.

Coroutines, simply put, are like normal functions except that they can be suspended (and resumed)
in the middle. When a coroutine is suspended, execution returns to the function that has called
the coroutine. If that function is also a coroutine and is waiting (`co_await`ing) for the current
coroutine to finish, then it is suspended as well and the execution returns to the function that has
called that coroutine and so on, until a function that is an actual function (not a coroutine) is reached.
In case of a regular Qt program, this "top-level" non-coroutine function will be the Qt's event loop -
which means that while your coroutine, when called from the Qt event loop is suspended, the Qt event
loop will continue to run until the coroutine is resumed again.

Amongst many other things, this allows you to write asynchronous code as if it were synchronous without
blocking the Qt event loop and making your application unresponsive. See the different examples in this
document.

Now let's look at the `co_await` keyword. This keyword tells the compiler that this is the point where
the coroutine wants to be suspended, until the *awaited* object (the *awaitable*) is ready. Anything type
can be *awaitable* - either because it directly implements the interface needed by the C++ coroutine
machinery, or because some external tools (like this library) are provided to wrap that type into something
that implements the *awaitable* interface.

The C++ coroutines introduce two additional keywords -`co_return` and `co_yield`:

From an application programmer point of view, `co_return` behaves exactly the same as `return`, except that
you cannot use the regular `return` in coroutines. There are some major differences under the hood, though,
which is likely why there's a special keyword for returning from coroutines.

`co_yield` allows a coroutine to produce a result without actually returning. Can be used for writing
generators. Currently, this library has no support/usage of `co_yield`, so I won't go into more details
here.

## Features

With a single exception of `QCoro::Task`, which we will discuss later, this library does not have any
classes, functions or types that users of this library need to use directly. All this library does is
providing behind-the-scenes machinery for the C++ coroutine system. The machinery is automatically
invoked when you use `co_await` with one of the supported Qt types.

The major benefit of using coroutines with Qt types is that it allows writing asynchronous code as if it
were synchronous and, most importantly, while the coroutine is `co_await`ing, the __Qt event loop runs
as usual__, meaning that your application remains responsive.

### `co_await QNetworkReply`

The `coro/network.h` header allows `co_await`ing on `QNetworkReply`.

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

### `co_await QDBusPendingCall`

The `coro/dbus.h` header allows `co_await`ing on `QDBusPendingCall`s.

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

### `co_await QFuture<T>`

The `coro/future.h` header allows `co_await`ing on `QFuture`.

```cpp
#include <qcoro/future.h>

QCoro::Task<> runTask() {
    // Starts a concurrent task and co_awaits on the returned QFuture. While the task is
    // running, the coroutine is suspended.
    const QString value = co_await QtConcurrent::run([]() {
        QString result;
        ...
        ...
        return result;
    });
    // When the future has finished, the coroutine is resumed and the result value of the
    // QFuture is returned.

    // ... now do something with the value
}
```

## `QCoro::Task`

You may have noticed in the examples above that our coroutines don't return `void` like they would
if they were normal functions. Instead they return `QCoro::Task<>` and you may be asking what it
is and why it's there.

We have already established that whenever we use `co_await` in a function, we turn that function
into a coroutine. And coroutines must return something called a promise type - it's an object that is
returned to the caller of the function. Not to be confused with `std::promise`, it has nothing to do
with this. The coroutine promise type allows the caller to control the coroutine.

Now all you need to know is, that in order for the magic contained in this repository to work your coroutine
must return the type `QCoro::Task<T>` - where `T` is the true return type of your function.

If you are calling a coroutine, to obtain the actual result value you must once again `co_await` it.

## Using QCoro

Using QCoro is easy and only requires a few steps:

1) Get QCoro, build and install it

```shell
git clone https://github.com/danvratil/qcoro.git
cd qcoro
mkdir build
cd build
cmake ..
make
# This will install QCoro into /usr/local/ prefix, change it by passing -DCMAKE_INSTALL_PREFIX=/usr
# to the cmake command above.
sudo make install
```

Alternatively, if you don't want to install QCoro into your system, you can just clone the repository (or add 
it as a git submodule) to your project. Since QCoro is a header-only library, you just need to update your
project's include paths to pick up the `qcoro` header directory.

2) Add it to your CMake:

```cmake
find_package(QCoro REQUIRED)
```

3) Use it in your code

```cpp
// If you want to use co_await with QDBusPendingCall...
#include <qcoro/dbus.h>
// If you want to use co_await with QNetworkReply...
#include <qcoro/network.h>
// If you want to use co_await with QFuture...
#include <qcoro/future.h>
// ... you get the idea.
```

And that's it!

## Compiler Support

I have tested everything with GCC 10.2.1 and Clang 11 on Fedora 33, but this library should be usable on any
decent Linux distribution, as long as it has a compiler that supports coroutines.

When using Clang, you must also use libc++ - using Clang with libstdc++ won't work as of now. Mostly because in
GCC, coroutines are fully supported while in Clang they are still considered experimental.

If you are willing to help with supporting/testing on MSVC, help is more than welcomed - this includes adding
support for some CI/CD.

## Thank You

This library is inspired by Lewis Bakers' cppcoro library, which also served as a guide to implementing the coroutine
machinery, alongside his great series on C++ coroutines:
 * [Coroutine Theory](https://lewissbaker.github.io/2017/09/25/coroutine-theory)
 * [Understanding Operator co_await](https://lewissbaker.github.io/2017/11/17/understanding-operator-co-await)
 * [Understanding the promise type](https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type)
 * [Understanding Symmetric Transfer](https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer)


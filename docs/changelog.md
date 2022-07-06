<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Changelog

## 0.6.0 (2022-07-09)

This release brings several major new features alongside a bunch of bugfixes and
improvements inside QCoro.

The three major features are:

* Generator support
* New QCoroWebSockets module
* Deprecated task.h
* Clang-cl and apple-clang support

🎉 Starting with 0.6.0 I no longer consider this library to be experimental
(since clearly the experiment worked :-)) and its API to be stable enough for
general use. 🎉

As always, big thank you to everyone who report issues and contributed to QCoro.
Your help is much appreciated!

### Generator support

Unlike regular functions (or `QCoro::Task<>`-based coroutines) which can only ever
produce at most single result (through `return` or `co_return` statement), generators
can yield results repeatedly without terminating. In QCoro we have two types of generators:
synchronous and asynchronous. Synchronous means that the generator produces each value
synchronously. In QCoro those are implemented as `QCoro::Generator<T>`:

```cpp
// A generator that produces a sequence of numbers from 0 to `end`.
QCoro::Generator<int> sequence(int end) {
    for (int i = 0; i <= end; ++i) {
        // Produces current value of `i` and suspends.
        co_yield i;
    }
    // End the iterator
}

int sumSequence(int end) {
    int sum = 0;
    // Loops over the returned Generator, resuming the generator on each iterator
    // so it can produce a value that we then consume.
    for (int value : sequence(end)) {
        sum += value;
    }
    return sum;
}
```

The `Generator` interface implements `begin()` and `end()` methods which produce an
iterator-like type. When the iterator is incremented, the generator is resumed to yield
a value and then suspended again. The iterator-like interface is not mandated by the C++
standard (the C++ standard provides no requirements for generators), but it is an
intentional design choice, since it makes it possible to use the generators with existing
language constructs as well as standard-library and Qt features.

You can find more details about synchronous generators in the [`QCoro::Generator<T>`
documentation](https://qcoro.dvratil.cz/reference/coro/generator/).

Asynchronous generators work in a similar way, but they produce value asynchronously,
that is the result of the generator must be `co_await`ed by the caller.

```cpp
QCoro::AsyncGenerator<QUrl> paginator(const QUrl &baseUrl) {
  QUrl pageUrl = baseUrl;
  Q_FOREVER {
    pageUrl = co_await getNextPage(pageUrl); // co_awaits next page URL
    if (pageUrl.isNull()) { // if empty, we reached the last page
      break; // leave the loop
    }
    co_yield pageUrl; // finally, yield the value and suspend
  }
  // end the generator
}

QCoro::AsyncGenerator<QString> pageReader(const QUrl &baseUrl) {
  // Create a new generator
  auto generator = paginator(baseUrl);
  // Wait for the first value
  auto it = co_await generator.begin();
  auto end = generator.end();
  while (it != end) { // while the `it` iterator is valid...
    // Asynchronously retrieve the page content
    const auto content = co_await fetchPageContent(*it);
    // Yield it to the caller, then suspend
    co_yield content;
    // When resumed, wait for the paginator generator to produce another value
    co_await ++it;
  }
}

QCoro::Task<> downloader(const QUrl &baseUrl) {
  int page = 1;
  // `QCORO_FOREACH` is like `Q_FOREACH` for asynchronous iterators
  QCORO_FOREACH(const QString &page, pageReader(baseUrl)) {
    // When value is finally produced, write it to a file
    QFile file(QStringLiteral("page%1.html").arg(page));
    file.open(QIODevice::WriteOnly);
    file.write(page);
    ++page;
  }
}
```

Async generators also have `begin()` and `end()` methods which provide an asynchronous
iterator-like types. For one, the `begin()` method itself is a coroutine and must be
`co_await`ed to obtain the initial iterator. The increment operation of the iterator
must then be `co_await`ed as well to obtain the iterator for the next value.
Unfortunately, asynchronous iterator cannot be used with ranged-based for loops, so
QCoro provides [`QCORO_FOREACH` macro](https://qcoro.dvratil.cz/reference/coro/asyncgenerator/#qcoro_foreach) to make using asynchronous generators simpler.

Read the [documentation for `QCoro::AsyncGenerator<T>`](https://qcoro.dvratil.cz/reference/coro/asyncgenerator) for more details.

### New QCoroWebSockets module

The QCoroWebSockets module provides QCoro wrappers for `QWebSocket` and `QWebSocketServer`
classes to make them usable with coroutines. Like the other modules, it's a standalone
shared or static library that you must explicitly link against in order to be able to use
it, so you don't have to worry that QCoro would pull websockets dependency into your
project if you don't want to.

```cpp
QCoro::Task<> ChatApp::handleNotifications(const QUrl &wsServer) {
  if (!co_await qCoro(mWebSocket).open(wsServer)) {
    qWarning() << "Failed to open websocket connection to" << wsServer << ":" << mWebSocket->errorString();
    co_return;
  }
  qDebug() << "Connected to" << wsServer;

  // Loops whenever a message is received until the socket is disconnected
  QCORO_FOREACH(const QString &rawMessage, qCoro(mWebSocket).textMessages()) {
    const auto message = parseMessage(rawMessage);
    switch (message.type) {
      case MessageType::ChatMessage:
        handleChatMessage(message);
        break;
      case MessageType::PresenceChange:
        handlePresenceChange(message);
        break;
      case MessageType::Invalid:
        qWarning() << "Received an invalid message:" << message.error;
        break;
    }
  }
}
```
The `textMessages()` methods returns an asynchronous generator, which yields the message
whenever it arrives. The messages are received and enqueued as long as the generator
object exists. The difference between using a generator and just `co_await`ing the next
emission of the `QWebSocket::textMessage()` signal is that the generator holds a connection
to the signal for its entire lifetime, so no signal emission is lost. If we were only
`co_await`ing a singal emission, any message that is received before we start `co_await`ing
again after handling the current message would be lost.

You can find more details about the `QCoroWebSocket` and `QCoroWebSocketSever`
in the [QCoro's websocket module documentation](https://qcoro.dvratil.cz/reference/websockets/).

You can build QCoro without the WebSockets module by passing `-DQCORO_WITH_QTWEBSOCKETS=OFF`
to CMake.

### Deprecated tasks.h header

The `task.h` header and it's camelcase variant `Task` been deprecated in QCoro 0.6.0
in favor of `qcorotask.h` (and `QCoroTask` camelcase version). The main reasons are to
avoid such a generic name in a library and to make the name consistent with the rest of
QCoro's public headers which all start with `qcoro` (or `QCoro`) prefix.

The old header is still present and fully functional, but including it will produce a
warning that you should port your code to use `qcorotask.h`. You can suppress the warning
by defining `QCORO_NO_WARN_DEPRECATED_TASK_H` in the compiler definitions:

CMake:
```cmake
add_compiler_definitions(QCORO_NO_WARN_DEPRECATED_TASK_H)
```

QMake
```qmake
DEFINES += QCORO_NO_WARN_DEPRECATED_TASK_H
```

The header file will be removed at some point in the future, at latest in the 1.0 release.

You can also pass `-DQCORO_DISABLE_DEPRECATED_TASK_H=ON` to CMake when compiling QCoro
to prevent it from installing the deprecated task.h header.

### Clang-cl and apple-clang support

The clang compiler is fully supported by QCoro since 0.4.0. This version of QCoro
intruduces supports for clang-cl and apple-clang.

Clang-cl is a compiler-driver that provides MSVC-compatible command line options,
allowing to use clang and LLVM as a drop-in replacement for the MSVC toolchain.

Apple-clang is the official build of clang provided by Apple on MacOS, which may be
different from the upstream clang releases.

### Full changelog

* Enable exceptions when compiling with clang-cl ([#90](https://github.com/danvratil/qcoro/issues/90), [#91](https://github.com/danvratil/qcoro/pull/91))
* Add option to generate code coverage report ([commit 0f0408c](https://github.com/danvratil/qcoro/commit/0f0408ce927e50450ab847cf290dd229b2a6e12c))
* Lower CMake requirement to 3.18.4 ([commit deb80c1](https://github.com/danvratil/qcoro/commit/deb80c13d9c9d866304fd5a64a33168adab34111))
* Add support for clang-cl ([#84](https://github.com/danvratil/qcoro/issues/84), [#86](https://github.com/danvratil/qcoro/pull/86))
* Avoid identifiers that begin with underscore and uppercase letter ([#83](https://github.com/danvratil/qcoro/pull/83))
* Add mising `<chrono>` include ([#82](https://github.com/danvratil/qcoro/pull/82)
* New module: QCoroWebSockets ([#75](https://github.com/danvratil/qcoro/pull/75), [#88](https://github.com/danvratil/qcoro/pull/88), [#89](https://github.com/danvratil/qcoro/pull/89))
* Add `QCoroFwd` header with forward-declarations of relevant types ([#71](https://github.com/danvratil/qcoro/issues/71))
* Deprecate `task.h` header file in favor of `qcorotask.h` ([#70](https://github.com/danvratil/qcoro/pull/70))
* Fix installing export headers ([#77](https://github.com/danvratil/qcoro/pull/77))
* Introduce support for generator coroutines ([#69](https://github.com/danvratil/qcoro/pulls/69))
* QCoro is now build with "modern Qt" compile definitions ([#66](https://github.com/danvratil/qcoro/pull/66))
* Export QCoro wrapper classes ([#63](https://github.com/danvratil/qcoro/issues/63), [#65](https://github.com/danvratil/qcoro/pull/65))
* Extended CI to include MSVC, apple-clang and multiple version of gcc and clang-cl ([#60](https://github.com/danvratil/qcoro/pull/60), [#61](https://github.com/danvratil/qcoro/pull/61))
* Fixed build with apple-clang


## 0.5.1 (2022-04-27)

* Fix build with GCC>=11.3 ([#57](https://github.com/danvratil/qcoro/issues/57), [#58](https://github.com/danvratil/qcoro/pulls/58))
* Fix ODR violation when building with GCC and LTO enabled ([#59](https://github.com/danvratil/qcoro/pulls/59))

## 0.5.0 (2022-04-25)

Major features:

* .then() continuation for `Task<T>`
* All asynchronous operations now return `Task<T>`
* Timeouts for many operations
* Support for `QThread`

### .then() continuation for Task<T>

Sometimes it's not possible to `co_await` a coroutine - usually because you need to integrate with a 3rd party code
that is not coroutine-ready. A good example might be implementing `QAbstractItemModel`, where none of the virtual
methods are coroutines and thus it's not possible to use `co_await` in them.

To still make it possible to all coroutines from such code, `QCoro::Task<T>` now has a new method: `.then()`,
which allows attaching a continuation callback that will be invoked by QCoro when the coroutine represented
by the `Task` finishes.

```cpp
void notACoroutine() {
    someCoroutineReturningQString().then([](const QString &result) {
        // Will be invoked when the someCoroutine() finishes.
        // The result of the coroutine is passed as an argument to the continuation.
    });
}
```

The continuation itself might be a coroutine, and the result of the `.then()` member function is again a `Task<R>`
(where `R` is the return type of the continuation callback), so it is possible to chain multiple continuations
as well as `co_await`ing the entire chain.

### All asynchronous operations now return `Task<T>`

Up until now each operation from the QCoro wrapper types returned a special awaitable  - for example,
`QCoroIODevice::read()` returned `QCoro::detail::QCoroIODevice::ReadOperation`. In most cases users of QCoro do
not need to concern themselves with that type, since they can still directly `co_await` the returned awaitable.

However, it unnecessarily leaks implementation details of QCoro into public API and it makes it harded to return
a coroutine from a non-coroutine function.

As of QCoro 0.5.0, all the operations now return `Task<T>`, which makes the API consistent. As a secondary effect,
all the operations can have a chained continuation using the `.then()` continuation, as described above.

### Timeout support for many operations

Qt doesn't allow specifying timeout for many operations, because they are typically non-blocking. But the timeout
makes sense in most QCoro cases, because they are combination of wait + the non-blocking operation. Let's take
`QIODevice::read()` for example: the Qt version doesn't have any timeout, because the call will never block - if
there's nothing to read, it simply returns an empty `QByteArray`.

On the other hand, `QCoroIODevice::read()` is an asynchronous operation, because under to hood, it's a coroutine
that asynchronously calls a sequence of

```cpp
device->waitForReadyRead();
device->read();
```

Since `QIODevice::waitForReadyRead()` takes a timeout argument, it makes sense for `QCoroIODevice::read()`
to also take (an optional) timeout argument. This and many other operations have gained support for timeout.

### Support for `QThread`

It's been a while since I added a new wrapper for a Qt class, so QCoro 0.5.0 adds wrapper for `QThread`. It's
now possible to `co_await` thread start and end:

```cpp
std::unique_ptr<QThread> thread(QThread::create([]() {
    ...
});
ui->setLabel(tr("Starting thread...");
thread->start();
co_await qCoro(thread)->waitForStarted();
ui->setLabel(tr("Calculating..."));
co_await qCoro(thread)->waitForFinished();
ui->setLabel(tr("Finished!"));
```

### Full changelog

* `.then()` continuation for `Task<T>` ([#39](https://github.com/danvratil/qcoro/pull/39))
* Fixed namespace scoping ([#45](https://github.com/danvratil/qcoro/pull/45))
* Fixed `QCoro::waitFor()` getting stuck when coroutine returns synchronously ([#46](https://github.com/danvratil/qcoro/pull/46))
* Fixed -pthread usage in CMake ([#47](https://github.com/danvratil/qcoro/pull/47))
* Produce QMake config files (.pri) for each module ([commit e215616](https://github.com/danvratil/qcoro/commit/e215616be8174438e907710025a7bd71e66a64b5))
* Fix build on platforms where -latomic must be linked explicitly ([#52](https://github.com/danvratil/qcoro/pull/52))
* Return `Task<T>` from all operations ([#54](https://github.com/danvratil/qcoro/pull/54))
* Add QCoro wrapper for `QThread` ([commit 832d931](https://github.com/danvratil/qcoro/commit/832d931068312c906db6858493fc952b8d984b1c))
* Many documentation updates

Thanks to everyone who contributed to QCoro!

## 0.4.0 (2022-01-06)

Major highlights in this release:

* Co-installability of Qt5 and Qt6 builds of QCoro
* Complete re-work of CMake configuration
* Support for compiling QCoro with Clang against libstdc++

### Co-installability of Qt5 and Qt6 builds of QCoro

This change mostly affects packagers of QCoro. It is now possible to install both Qt5 and Qt6 versions
of QCoro alongside each other without conflicting files. The shared libraries now contain the Qt version
number in their name (e.g. `libQCoro6Core.so`) and header files are also located in dedicated subdirectories
(e.g. `/usr/include/qcoro6/{qcoro,QCoro}`). User of QCoro should not need to do any changes to their codebase.

### Complete re-work of CMake configuration

This change affects users of QCoro, as they will need to adjust CMakeLists.txt of their projects. First,
depending on whether they want to use Qt5 or Qt6 version of QCoro, a different package must be used.
Additionally, list of QCoro components to use must be specified:

```
find_package(QCoro5 REQUIRED COMPONENTS Core Network DBus)
```

Finally, the target names to use in `target_link_libraries` have changed as well:

* `QCoro::Core`
* `QCoro::Network`
* `QCoro::DBus`

The version-less `QCoro` namespace can be used regardless of whether using Qt5 or Qt6 build of QCoro.
`QCoro5` and `QCoro6` namespaces are available as well, in case users need to combine both Qt5 and Qt6
versions in their codebase.

This change brings QCoro CMake configuration system to the same style and behavior as Qt itself, so it
should now be easier to use QCoro, especially when supporting both Qt5 and Qt6.

### Support for compiling QCoro with Clang against libstdc++

Until now, when the Clang compiler was detected, QCoro forced usage of LLVM's libc++ standard library.
Coroutine support requires tight co-operation between the compiler and standard library. Because Clang
still considers their coroutine support experimental it expects all coroutine-related types in standard
library to be located in `std::experimental` namespace. In GNU's libstdc++, coroutines are fully supported
and thus implemented in the `std` namespace. This requires a little bit of extra glue, which is now in place.

### Full changelog

* QCoro can now be built with Clang against libstdc++ ([#38](https://github.com/danvratil/qcoro/pull/38), [#22](https://github.com/danvratil/qcoro/issues/22))
* Qt5 and Qt6 builds of QCoro are now co-installable ([#36](https://github.com/danvratil/qcoro/issues/36), [#37](https://github.com/danvratil/qcoro/pull/37))
* Fixed early co_return not resuming the caller ([#24](https://github.com/danvratil/qcoro/issue/24), [#35](https://github.com/danvratil/qcoro/pull/35))
* Fixed QProcess example ([#34](https://github.com/danvratil/qcoro/pull/34))
* Test suite has been improved and extended ([#29](https://github.com/danvratil/qcoro/pull/29), [#31](https://github.com/danvratil/qcoro/pull/31))
* Task move assignment operator checks for self-assignment ([#27](https://github.com/danvratil/qcoro/pull/27))
* QCoro can now be built as a subdirectory inside another CMake project ([#25](https://github.com/danvratil/qcoro/pull/25))
* Fixed QCoroCore/qcorocore.h header ([#23](https://github.com/danvratil/qcoro/pull/23))
* DBus is disabled by default on Windows, Mac and Android ([#21](https://github.com/danvratil/qcoro/pull/21))

Thanks to everyone who contributed to QCoro!

## 0.3.0 (2021-10-11)

* Added SOVERSION to shared libraries ([#17](https://github.com/danvratil/qcoro/pull/17))
* Fixed building tests when not building examples ([#19](https://github.com/danvratil/qcoro/pull/19))
* Fixed CI

Thanks to everyone who contributed to QCoro 0.3.0!

## 0.2.0 (2021-09-08)

### Library modularity

The code has been reorganized into three modules (and thus three standalone libraries): QCoroCore, QCoroDBus and
QCoroNetwork. QCoroCore contains the elementary QCoro tools (`QCoro::Task`, `qCoro()` wrapper etc.) and coroutine
support for some QtCore types. The QCoroDBus module contains coroutine support for types from the QtDBus module
and equally the QCoroNetwork module contains coroutine support for types from the QtNetwork module. The latter two
modules are also optional, the library can be built without them. It also means that an application that only uses
let's say QtNetwork and has no DBus dependency will no longer get QtDBus pulled in through QCoro, as long as it
only links against `libQCoroCore` and `libQCoroNetwork`. The reorganization will also allow for future
support of additional Qt modules.

### Headers clean up

The include headers in QCoro we a bit of a mess and in 0.2.0 they all got a unified form. All public header files
now start with `qcoro` (e.g. `qcorotimer.h`, `qcoronetworkreply.h` etc.), and QCoro also provides CamelCase headers
now. Thus users should simply do `#include <QCoroTimer>` if they want coroutine support for `QTimer`.

The reorganization of headers makes QCoro 0.2.0 incompatible with previous versions and any users of QCoro will
have to update their `#include` statements. I'm sorry about this extra hassle, but with this brings much needed
sanity into the header organization and naming scheme.

### Docs update

The documentation has been updated to reflect the reorganization as well as some internal changes. It should be
easier to understand now and hopefully will make it easier for users to start with QCoro now.

### Internal API cleanup and code de-duplication

Historically, certain types types which can be directly `co_await`ed with QCoro, for instance `QTimer` has their
coroutine support implemented differently than types that have multiple asynchronous operations and thus have
a coroutine-friendly wrapper classes (like `QIODevice` and it's `QCoroIODevice` wrapper). In 0.2.0 I have unified
the code so that even the coroutine support for simple types like `QTimer` are implemented through wrapper classes
(so there's `QCoroTimer` now)

## 0.1.0 (2021-08-15)

* Initial release QCoro

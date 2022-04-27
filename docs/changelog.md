<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Changelog

## 0.5.1 (2022-04-27)

* Fix build with GCC>=11.3 ([#57](https://github.com/danvratil/qcoro/issues/#57), [#58](https://github.com/danvratil/qcoro/pulls/58))
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

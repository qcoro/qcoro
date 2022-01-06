# Changelog

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

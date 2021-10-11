# Changelog

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

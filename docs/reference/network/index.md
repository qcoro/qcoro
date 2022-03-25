<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Network Module

The `Network` module contains coroutine-friendly wrapper for
[QtNetwork][qtdoc-qtnetwork] classes.

## CMake Usage

```
find_package(QCoro6 COMPONENTS Network)

...

target_link_libraries(my-target QCoro::Network)
```

## QMake Usage

```
QT += QCoroNetwork
```

[qtdoc-qtnetwork]: https://doc.qt.io/qt-5/qtnetwork-index.html

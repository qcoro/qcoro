<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# WebSockets Module

The `WebSockets` module contains coroutine-friendly wrapper for
[QtWebSockets][qtdoc-websockets] classes.

## CMake Usage

```
find_package(QCoro6 COMPONENTS WebSockets)

...
target_link_libraries(my-target QCoro::WebSockets)
```

## QMake Usage

```
QT += QCoroWebSockets
```

[qtdoc-websockets]: https://doc.qt.io/qt-5/qtwebsockets-index.html

<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Core Module

The `Core` module contains coroutine-friendly wrapper for
[QtCore][qtdoc-qtcore] classes.

## CMake Usage

```
find_package(QCoro6 COMPONENTS Core)

...

target_link_libraries(my-target QCoro::Core)
```

## QMake Usage

```
QT += QCoroCore
```


[qtdoc-qtcore]: https://doc.qt.io/qt-5/qtcore-index.html

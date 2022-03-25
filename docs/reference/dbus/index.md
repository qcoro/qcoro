<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# DBus Module

The `DBus` module contains coroutine-friendly wrapper for
[QtDBus][qtdoc-qtdbus] classes.

## CMake Usage

```
find_package(QCoro6 COMPONENTS DBus)

...

target_link_libraries(my-target QCoro::DBus)
```

## QMake Usage

```
QT += QCoroDBus
```

[qtdoc-qtdbus]: https://doc.qt.io/qt-5/qtdbus-index.html


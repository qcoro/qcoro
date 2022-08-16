<!--
SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Quick Module

The `Quick` module contains coroutine-friendly wrappers for
[QtQuick][qtdoc-qtquick] classes.

## CMake Usage

```cmake
find_package(QCoro6 COMPONENTS Quick)
...
target_link_libraries(my-target QCoro::Quick)
```

## QMake Usage

```
QT += QCoroQuick
```

[qtdoc-qtquick]: https://doc.qt.io/qt-5/qtquick-index.html

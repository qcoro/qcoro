<!--
SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QML Module

The `QML` module contains coroutine-friendly wrappers for
[QtQml][qtdoc-qml] classes.

## CMake Usage

```cmake
find_package(QCoro6 COMPONENTS Qml)
...
target_link_libraries(my-target QCoro::Qml)
```

## QMake Usage

```
QT += QCoroQml
```

## Type registration

To use types defined in QCoroQml, you need to call the `QCoro::Qml::registerTypes` function before loading the QML.

```C++
int main() {
    ...
    QCoro::Qml::registerTypes();
    ...
}
```

[qtdoc-qml]: https://doc.qt.io/qt-5/qml-index.html

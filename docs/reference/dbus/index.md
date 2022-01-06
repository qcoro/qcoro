# DBus Module

The `DBus` module contains coroutine-friendly wrapper for
[QtDBus][qtdoc-qtdbus] classes.

## CMake usage

```
find_package(QCoro6 COMPONENTS DBus)

...

target_link_libraries(my-target QCoro::DBus)
```


[qtdoc-qtdbus]: https://doc.qt.io/qt-5/qtdbus-index.html


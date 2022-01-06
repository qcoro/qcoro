# Network Module

The `Network` module contains coroutine-friendly wrapper for
[QtNetwork][qtdoc-qtnetwork] classes.

## CMake usage

```
find_package(QCoro6 COMPONENTS Network)

...

target_link_libraries(my-target QCoro::Network)
```

[qtdoc-qtnetwork]: https://doc.qt.io/qt-5/qtnetwork-index.html

# Building and Using QCoro

## Building QCoro

QCoro uses CMake build system. You can pass following options to the `cmake` command when building
QCoro to customize the build:

* `-DQCORO_BUILD_EXAMPLES` - whether to build examples or not (`ON` by default).
* `-DQCORO_ENABLE_ASAN` - whether to build QCoro with AddressSanitizer (`OFF` by default).
* `-DBUILD_SHARED_LIBS` - whether to build QCoro as a shared library (`OFF` by default).
* `-DBUILD_TESTING` - whether to build tests (`ON` by default).
* `-DUSE_QT_VERSION` - set to `5` or `6` to force a particular version of Qt. When not set the highest available version is used.


```
mkdir build
cd build
cmake .. <CMAKE FLAGS>
make
# This will install QCoro into /usr/local/ prefix, change it by passing -DCMAKE_INSTALL_PREFIX=/usr
# to the cmake command above.
sudo make install
```

## Add it to your CMake

```cmake
find_package(QCoro REQUIRED)

# Set necessary compiler flags to enable coroutine support
qcoro_enable_coroutines()

...

target_link_libraries(your-target QCoro::QCoro)
```



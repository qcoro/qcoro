# QCoro

C++ Coroutine library for Qt

---

## Overview

QCoro is a C++ library that provide set of tools to make use of C++20 coroutines
in connection with certain asynchronous Qt actions.

Take a look at the example below to see what an amazing thing coroutines are:
```cpp
QNetworkAccessManager networkAccessManager;
// co_await the reply - the coroutine is suspended until the QNetworkReply is finished.
// While the coroutine is suspended, *the Qt event loop runs as usual*.
const QNetworkReply *reply = co_await networkAccessManager.get(url);
// Once the reply is finished, your code resumes here as if nothing amazing has just happened ;-)
const auto data = reply->readAll();
```

This library has only one class and one function that the user must be aware of: the class is
[`QCoro::Task`](reference/task.md) and must be used as a return type for any coroutine that `co_await`s
a Qt type. The function is [`qCoro()`](reference/coro.md) and it provides coroutine-friendly
wrappers for Qt types that have multiple asynchronous operations that the user may want to `co_await`
(for example [`QProcess`](reference/qprocess.md)). All the other code (basically everything in the
`QCoro::detail` namespace) is here to provide the cogs and gears for the C++ coroutine machinery,
making it possible to use Qt types with coroutines.

The major benefit of using coroutines with Qt types is that it allows writing asynchronous code as if it
were synchronous and, most importantly, while the coroutine is `co_await`ing, the __Qt event loop runs
as usual__, meaning that your application remains responsive.

This is a rather experimental library that I started working on to better understand coroutines
in C++. After reading numerous articles and blog posts about coroutines, it still wasn't exactly
clear to me how the whole thing works, so I started working on this library to get a better idea
about coroutines.

## Coroutines

Coroutines are regular functions, except that they can be suspended and resumed again. When
a coroutine is suspended, it returns sort of a promise to the caller and the caller continues
executing their code. At some point, the caller can use the newly introduced `co_await` keyword
to wait for the returned promise to be fulfilled. When that happens, the caller is suspended
and instead the coroutine is resumed.

This allows writing asynchronous code as if it were synchronous, making it much easier to read
and understand.

That's not all that coroutines can do, you can read more about it in the 'Coroutines' section
of this documentation.

## Supported Compilers

This library requires a compiler that supports the Coroutine TS (obviously). Currently
GCC and Clang are supported, MSVC should be possible too, but I haven't tried.

All examples were tested with GCC 10 and Clang 11, although even slightly older versions
should work.

In both GCC and Clang, coroutine support must be explicitly enabled:

### GCC

To enable coroutines support in GCC, add `-fcoroutines` to `CXX_FLAGS`.

CMake:
```
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
```

### Clang

In Clang coroutines are still considered experimental (unlike in GCC), so
you cannot mix Clang and libstdc++. You must use Clang with libc++. Coroutines
are enabled by adding `-fcoroutines-ts` to `CMAKE_CXX_FLAGS`.

CMake:
```
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts -stdlib=libc++")
```

### MSVC

Currently not supported by QCoro, although MSVC does have full implementation of the
Coroutines TS. It should be fairly easy to make QCoro work with MSVC, though. If you
are willing to help, please od get in touch.

## Using QCoro

Using QCoro is easy and only requires a few steps:

1) Get QCoro, build and install it

```shell
git clone https://github.com/danvratil/qcoro.git
cd qcoro
mkdir build
cd build
cmake ..
make
# This will install QCoro into /usr/local/ prefix, change it by passing -DCMAKE_INSTALL_PREFIX=/usr
# to the cmake command above.
sudo make install
```

Alternatively, if you don't want to install QCoro into your system, you can just clone the repository (or add 
it as a git submodule) to your project. Since QCoro is a header-only library, you just need to update your
project's include paths to pick up the `qcoro` header directory.

2) Add it to your CMake:

```cmake
find_package(QCoro REQUIRED)
...
target_link_libraries(your-target QCoro::qcoro)
```

3) Use it in your code

See the Reference section for detail.

And that's it!


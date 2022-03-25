<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QCoro

C++ Coroutine Library for Qt5 and Qt6

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
[`QCoro::Task`](reference/coro/task.md) and must be used as a return type for any coroutine that `co_await`s
a Qt type. The function is [`qCoro()`](reference/coro/coro.md) and it provides coroutine-friendly
wrappers for Qt types that have multiple asynchronous operations that the user may want to `co_await`
(for example [`QProcess`](reference/core/qprocess.md)). All the other code (basically everything in the
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
GCC, Clang and MSVC are supported.

All examples were tested with GCC 10 and Clang 11, although even slightly older versions
should work.

In both GCC and Clang, coroutine support must be explicitly enabled.

### GCC

To enable coroutines support in GCC, add `-fcoroutines` to `CXX_FLAGS`.

CMake:
```
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
```

Alternatively, just use `qcoro_enable_coroutines()` CMake macro provided by QCoro to set the
flags automatically.

### Clang

In Clang coroutines are still considered experimental (unlike in GCC).
Coroutines are enabled by adding `-fcoroutines-ts` to `CMAKE_CXX_FLAGS`.

CMake:
```
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts")
```

Alternatively, just use `qcoro_enable_coroutines()` CMake macro provided by QCoro to set the
flags automatically.

### MSVC

Coroutine support in MSVC is enabled automatically by CMake when C++20 standard is specified
in `CMAKE_CXX_STANDARD`:

```
set(CMAKE_CXX_STANDARD 20)
```


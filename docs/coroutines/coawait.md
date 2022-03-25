<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# `co_await` Explained

The following paragraphs try to explain what is a coroutine and what `co_await` does in some
simple way. I don't guarantee that any of this is factically correct. For more gritty (and
correct) details, refer to the articles linked at the bottom of this document.

Coroutines, simply put, are like normal functions except that they can be suspended (and resumed)
in the middle. When a coroutine is suspended, execution returns to the function that has called
the coroutine. If that function is also a coroutine and is waiting (`co_await`ing) for the current
coroutine to finish, then it is suspended as well and the execution returns to the function that has
called that coroutine and so on, until a function that is an actual function (not a coroutine) is reached.
In case of a regular Qt program, this "top-level" non-coroutine function will be the Qt's event loop -
which means that while your coroutine, when called from the Qt event loop is suspended, the Qt event
loop will continue to run until the coroutine is resumed again.

Amongst many other things, this allows you to write asynchronous code as if it were synchronous without
blocking the Qt event loop and making your application unresponsive. See the different examples in this
document.

Now let's look at the `co_await` keyword. This keyword tells the compiler that this is the point where
the coroutine wants to be suspended, until the *awaited* object (the *awaitable*) is ready. Anything type
can be *awaitable* - either because it directly implements the interface needed by the C++ coroutine
machinery, or because some external tools (like this library) are provided to wrap that type into something
that implements the *awaitable* interface.

The C++ coroutines introduce two additional keywords -`co_return` and `co_yield`:

From an application programmer point of view, `co_return` behaves exactly the same as `return`, except that
you cannot use the regular `return` in coroutines. There are some major differences under the hood, though,
which is likely why there's a special keyword for returning from coroutines.

`co_yield` allows a coroutine to produce a result without actually returning. Can be used for writing
generators. Currently, this library has no support/usage of `co_yield`, so I won't go into more details
here.



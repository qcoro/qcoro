<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QCoro::Generator<T>

{{ doctable("Coro", "QCoroGenerator") }}

```cpp
template<typename T> class QCoro::Generator
```

Generator is a coroutine that that yields a single value and then
suspends itself, until resumed again. Then it yields another value
and suspends again. Generator can be inifinite (the coroutine will
never finish and will produce new values for ever until destroyed
from the outside) or finite (after generating some amount of values
the coroutine finishes).

The `QCoro::Generator<T>` is a template class providing interface
for the generator consumer. There is no standard API for generators
specified in the C++ standard (as of C++20). The design chosen by
QCoro copies the design of cppcoro library, which is for the `Generator<T>`
class to provide `begin()` and `end()` methods to obtain iterator-
like objects, allowing the generators to be used like containers and
providing an interface that is familiar to every C++ programmer.

```cpp
// Simple generator that produces `count` values from 0 to `(count-1)`.
QCoro::Generator<int> iota(int count) {
    for (int i = 0; i < count; ++i) {
        // Yields a value and suspends the generator.
        co_yield count;
    }
}

void counter() {
    // Creates a new initially suspended generator coroutine
    auto generator = iota(10);
    // Resumes the coroutine, obtains first value and returns
    // an iterator representing the first value.
    auto it = generator.begin();
    // Obtains a past-the-end iterator.
    const auto end = generator.end();

    // Loops until the generator doesn't finish.
    while (it != end) {
        // Resumes the generator until it co_yields another value.
        ++it;
        // Reads the current value from the iterator.
        std::cout << *it << std::endl;
    }

    // The code above can be written more consisely using ranged based for-loop
    for (const auto value : iota(10)) {
        std::count << value << std::endl;
    }
}
```

## Infinite generators

A generator may be inifinite, that is it may never finish and keep
producing values whenever queried. A simple naive example might be
a generator producing random value whenever resumed.

See the next chapter on generator lifetime regarding how to destroy
an infinite generator.


```cpp
QCoro::Generator<quint32> randomNumberGenerator() {
    auto *generator = QRandomGenerator::system();
    while (true) {
        // Generates a single random value and suspends.
        co_yield generator->generate();
    }
}

void randomInitialize(QVector<quint32> &vector) {
    // Constructs the generator
    auto rng = randomNumberGenerator();
    // Gets the first tandom value
    auto rngIt = rng.begin();
    // Loops over all values of the vector
    for (auto &val : vector) {
        // Stores the current random value and generates a next one
        val = *(rngIt++);
    }
} // Destroyes the generator coroutine.
```

## Generator lifetime

The lifetime of the generator coroutine is tight to the lifetime
of the associated `QCoro::Generator<T>` object. The generator
coroutine is destroyed when the associated `QCoro::Generator<T>`
object is destroyed, that includes the stack of the coroutine
and everything allocated on the stack.

It doesn't matter whether the coroutine has already finished or
whether it is suspended after yielding a value. When the
`QCoro::Generator<T>` object is destroyed, the stack of the
coroutine and all associated state will be destroyed using the
regular rules of stack destruction.

Therefore, it is stafe to allocate values on generator coroutine
stack. However, dynamically allocated memory will not be free'd
automatically. Therefore if you need to dynamically allocate
memory on heap inside the generator coroutine, you must make
sure to either free it before the generator coroutine is suspended,
or that it is destroyed when the stack is destroyed, e.g. by using
it in `std::unique_ptr` or `QScopeGuard`.

!!! info "Memory usage"
    Keep in mind, that that generator coroutine will keep occupying
    memory even when not used until it finishes or until the
    associated `QCoro::Generator<T>` is destroyed.

## Exceptions

When a generator coroutine throws an exception, it will be rethrown
from `operator++()` or from generator's `begin()` method.

Afterwards the iterator is considered invalid and the generator
is finished and may not be used anymore.

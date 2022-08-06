<!--
SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>

SPDX-License-Identifier: GFDL-1.3-or-later
-->

# QCoro::AsyncGenerator<T>

{{ doctable("Coro", "QCoroAsyncGenerator") }}

```cpp
template<typename T> class QCoro::AsyncGenerator
```

`AsyncGenerator<T>` is fundamentally identical to [`QCoro::Generator<T>`][qcoro-generator].
The only difference is that the value is produced asynchronously. Asynchronous
generator is used exactly the same way as synchronous generators, except that the
result of `AsyncGenerator<T>::begin()` must be `co_await`ed, and incrementing
the returned iterator must be `co_await`ed as well.

```cpp
QCoro::AsyncGenerator<uint8_t> lottoNumbersGenerator(int count) {
    Hat hat; // Hat with numbers
    Hand hand; // Hand to pull numbers out of the hat
    for (int = 0; i < count; ++i) {
        const uint8_t number = co_await hand.pullNumberFrom(hat);
        co_yield number; // guaranteed to be a winning number
    }
}

QCoro::Task<> winningLottoNumbers() {
    const auto makeMeRich = lottoNumbersGenerator(10);
    // We must co_await begin() to obtain the initial iterator
    auto winningNumber = co_await makeMeRich.begin();
    std::cout << "Winning numbers: ";
    while (winningNumber != makeMeRich.end()) {
        std::cout << *winningNumger;
        // And we must co_await increment
        co_await ++(winningNumber);
    }
}
```

You might be wondering why are we `co_await`ing `AsyncGenerator<T>::begin()` and
`AsyncGeneratorIterator<T>::operator++()`, and not just the value (the result of
dereferencing the iterator) - it would surely make the code simpler and more intuitive.
The simple reason is that we are `co_await`ing the next iterator (which either holds
a value or is past-the-end iterator) rather than the value itself. Once we have the
iterator, we must check whether it's valid or not, because paste-the-end iterator is
not dereferencable. That's why the `AsyncGenerator<T>::begin()` and
`AsyncGeneratorIterator<T>::operator++()` operations are asynchronous, rather than
`AsyncGeneratorIterator<T>::operator*()`.

#### Exceptions

When the generator coroutine throws an exception, the exception will be rethrown from
the `operator++()` (or from generator's `begin()`) function when they are `co_await`ed.

Afterwards, the iterator is considered invalid and the generator is finished and may not
be used anymore.

```cpp
QCoro::AsyncGenerator<int> generatorThatThrows() {
    while (true) {
        const int val = co_await generateRandomNumber();
        if (val == 0) {
            throw std::runtime("Division by zero!");
        }
        co_yield 42 / val;
    }
}

QCoro::Task<> divide42ForNoReason(std::size_t count) {
    auto generator = generatorThatThrows();
    std::vector<int> numbers;
    try {
        // Might throw if generator generates 0 immediatelly.
        auto it = co_await generator.begin();
        while (numbers.size() < count) {
            numbers.push_back(*it);
            co_await ++it; // might throw
        }
    } catch (const std::runtime_error &e) {
        // We were unable to generate all numbers
    }
}
```

### QCORO_FOREACH

```cpp
#define QCORO_FOREACH(value, generator)
```

The example in previous chapter shows one example of looping over values produced
by an asynchronous generator with a `while` loop. This is how it would look like
with a `for` loop:

```cpp
auto generator = createGenerator();
for (auto it = co_await generator.begin(), end = generator.end(); it != end; co_await ++it) {
    const QString &value = *it;
    ...//do something with value
}
```

Sadly, it's not possible to use the generator with a ranged-based for loop. While
initially the proposal for C++ coroutines did contain `for co_await` construct, it
was removed as the committee was [concerned that it was making assumptions about the
future interface of asynchronous generators][p0664r8c35].

For that reason, QCoro provides `QCORO_FOREACH` macro, which is very similar to
[`Q_FOREACH`][qdoc-qforeach] and works exactly like the for-loop in the example
above:

```cpp
QCORO_FOREACH(const QString &value, createGenerator()) {
    ... // do something with value
}
```

[qcoro-generator]: ./generator.md
[p0664r8c35]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0664r8.html#35
[qdoc-qforeach]: https://doc.qt.io/qt-5/qtglobal.html#Q_FOREACH

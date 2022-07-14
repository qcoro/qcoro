// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <tuple>

namespace QCoro {

namespace Options {
    struct AbortOnException;
} // namespace Options

template<typename T>
concept TaskOption =
    std::is_same_v<T, Options::AbortOnException>;

template<TaskOption ...Opts>
struct TaskOptions;

struct TaskOptionsStorage;

template<TaskOption ... Opts>
struct TaskOptions;

template<typename T = void, typename Opts = TaskOptions<>> class Task;
template<typename T> class Generator;
template<typename T> class GeneratorIterator;
template<typename T> class AsyncGenerator;
template<typename T> class AsyncGeneratorIterator;

} // namespace QCoro

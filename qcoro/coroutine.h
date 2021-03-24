// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once
#if defined(__clang__)
    #include <experimental/coroutine>
    #define QCORO_STD std::experimental
#elif defined(__GNUC__)
    #include <coroutine>
    #define QCORO_STD std
#else
    #pragma error "Current compiler doesn't support Coroutines."
#endif

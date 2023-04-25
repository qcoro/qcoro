// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

//! Executes given \c expr with 10ms delay.
#define QCORO_DELAY(expr)                                                                          \
    QTimer::singleShot(10ms, [&]() { expr; })

#define QCORO_TEST_TIMEOUT(expr) { \
    const auto start = std::chrono::steady_clock::now(); \
    const bool ok = expr; \
    const auto end = std::chrono::steady_clock::now(); \
    QCORO_VERIFY(!ok); \
    QCORO_VERIFY((end - start) < 500ms); \
}



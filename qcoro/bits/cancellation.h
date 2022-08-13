// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

namespace QCoro {

namespace detail {

struct Cancellation_t;
Cancellation_t cancellation() noexcept;

struct Cancellation_t {
private:
    friend Cancellation_t cancellation() noexcept;
    Cancellation_t() = default;
};

Cancellation_t cancellation() noexcept {
    return {};
}

} // namespace detail

} // namespace QCoro
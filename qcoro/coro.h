// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"
#include "wrappers/qprocess.h"

namespace QCoro {

inline auto coro(QProcess &p) noexcept { return detail::QProcessWrapper{&p}; }
inline auto coro(QProcess *p) noexcept { return detail::QProcessWrapper{p}; }

} // namespace QCoro



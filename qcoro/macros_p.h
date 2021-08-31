// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifndef QCORO_DEFAULT_MOVE
#define QCORO_DEFAULT_MOVE(Class)                                                                  \
    Class(Class &&) noexcept = default;                                                            \
    Class &operator=(Class &&) noexcept = default;
#endif

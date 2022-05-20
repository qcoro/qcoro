// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifndef QCORO_NO_WARN_DEPRECATED_TASK_H
    #if defined(__GNUC__) || defined(__clang__)
        #pragma message "The qcoro/task.h include header is deprecated and will be removed " \
                            "in some future version of QCoro. Please use qcorotask.h header instead. " \
                            "You can define QCORO_NO_WARN_DEPRECATED_TASK_H to suppress this warning."
    #elif defined(_MSC_VER)
        #pragma message( "The qcoro/task.h include header is deprecated and will be removed " \
                         "in some future version of QCoro. Please use qcorotask.h header instead. " \
                         "You can define QCORO_NO_WARN_DEPRECATED_TASK_H to suppress this warning." )
    #endif
#endif

#include "qcorotask.h"

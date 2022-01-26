// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorocore-config.h"

#include "qcoroiodevice.h"
#ifdef QCOROCORE_WITH_PROCESS
#include "qcoroprocess.h"
#endif
#include "qcorosignal.h"
#include "qcorotimer.h"
#ifdef QCOROCORE_WITH_FUTURE
#include "qcorofuture.h"
#endif


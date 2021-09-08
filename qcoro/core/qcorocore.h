// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "config.h"

#include "iodevice.h"
#include "qcoroiodevice.h"
#include "qcoroprocess.h"
#include "qcorosignal.h"
#include "timer.h"

#ifdef QCORO_QT_HAS_COMPAT_ABI
    #include "future.h"
#endif


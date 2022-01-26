// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoro-config.h"

#include "task.h"
#include "qcorocore.h"
#ifdef QCORO_WITH_QTDBUS
#include "qcorodbus.h"
#endif
#ifdef QCORO_WITH_QTNETWORK
#include "qcoronetwork.h"
#endif

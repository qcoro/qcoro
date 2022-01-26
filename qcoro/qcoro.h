// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <qglobal.h>

#include "task.h"
#include "qcorocore.h"
#if defined(QT_DBUS_LIB)
#  include "qcorodbus.h"
#endif
#include "qcoronetwork.h"

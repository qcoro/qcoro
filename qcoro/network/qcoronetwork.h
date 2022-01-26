// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoronetwork-config.h"

#include "qcoroabstractsocket.h"
#ifdef QCORONETWORK_WITH_LOCALSERVER
#include "qcorolocalsocket.h"
#endif
#include "qcoronetworkreply.h"
#include "qcorotcpserver.h"

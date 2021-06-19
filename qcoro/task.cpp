// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "task.h"

#include <QtCore/private/qobject_p.h>

using namespace QCoro;

// Keep this here: we don't want to expose Qt private includes headers and paths into our public
// interface.
QEvent *detail::Waiter::createEvent(QtPrivate::QSlotObjectBase *slot)
{
    return new QMetaCallEvent(slot, nullptr, -1, 1);
}

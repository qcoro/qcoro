// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: MIT

#include "qcoroqml.h"

#include "qcoroqmltask.h"

#include <QQmlApplicationEngine>

void QCoro::Qml::registerTypes() {
    qRegisterMetaType<QCoro::QmlTask>();
    qmlRegisterAnonymousType<QCoro::QmlTaskListener>("QCoro", 0);
}

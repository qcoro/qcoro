// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QIODevice>

namespace QCoro::detail {

class WaitSignalHelper : public QObject {
    Q_OBJECT
public:
    explicit WaitSignalHelper(const QIODevice *device, void(QIODevice::*signalFunc)());
    explicit WaitSignalHelper(const QIODevice *device, void(QIODevice::*signalFunc)(qint64));

Q_SIGNALS:
    void ready(bool result);
    void ready(qint64 result);

protected:
    template<typename T>
    void emitReady(T result) {
        cleanup();
        Q_EMIT this->ready(result);
    }

    virtual void cleanup() {
        disconnect(mReady);
        disconnect(mAboutToClose);
    }

    QMetaObject::Connection mReady;
    QMetaObject::Connection mAboutToClose;
};

} // namespace QCoro::detail
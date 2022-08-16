// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QQuickAsyncImageProvider>

#include <QCoro/QCoroTask>

#include "qcoroquick_export.h"

namespace QCoro {

//! Base class for coroutines based image providers
class QCOROQUICK_EXPORT ImageProvider : public QQuickAsyncImageProvider {
public:
    explicit ImageProvider();

    //! This function needs to be re-implemented in a subclass.
    virtual QCoro::Task<QImage> asyncRequestImage(const QString &id, const QSize &requestedSize) = 0;

private:
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};

}

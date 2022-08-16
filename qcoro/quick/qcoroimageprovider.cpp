// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: MIT

#include "qcoroimageprovider.h"

namespace QCoro {

// Internal implementation of the QQuickImageResponse interface
class QCoroImageResponse : public QQuickImageResponse {

public:
    QQuickTextureFactory *textureFactory() const override;
    void reportFinished(QImage &&image);

private:
    QImage m_image;
};


ImageProvider::ImageProvider() = default;

QQuickImageResponse *ImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize) {
    auto task = asyncRequestImage(id, requestedSize);

    auto *response = new QCoroImageResponse();

    task.then([response](QImage &&image) {
        response->reportFinished(std::move(image));
    });

    return response;
}

QQuickTextureFactory *QCoroImageResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void QCoroImageResponse::reportFinished(QImage &&image) {
    m_image = image;
    Q_EMIT finished();
}

}

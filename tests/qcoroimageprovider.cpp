// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/core/qcorotimer.h"
#include "qcoro/quick/qcoroimageprovider.h"

#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>

#define emit Q_EMIT // HACK: the private header uses `emit`, but QCoro is built with QT_NO_KEYWORDS
#include <private/qquickimage_p.h>
#undef emit

class TestImageProvider final: public QCoro::ImageProvider {
public:
    TestImageProvider(bool async, const QString &error)
        : mAsync(async), mError(error)
    {}

protected:
    QCoro::Task<QImage> asyncRequestImage(const QString &id, const QSize &requestedSize) override {
        Q_UNUSED(id);
        Q_UNUSED(requestedSize);

        if (mAsync) {
            QTimer timer;
            timer.start(250ms);
            co_await timer;
        }

        co_return QImage{};
    }

private:
    bool mAsync;
    QString mError;
};

class QCoroImageProviderTest: public QCoro::TestObject<QCoroImageProviderTest> {
    Q_OBJECT

private Q_SLOTS:
    void testImageProvider_data() {
        QTest::addColumn<QString>("id");
        QTest::addColumn<bool>("async");
        QTest::addColumn<QString>("error");

        QTest::newRow("sync") << QStringLiteral("image-sync.jpg") << false << QString();
        QTest::newRow("async") << QStringLiteral("image-async.jpg") << true << QString();
    }

    void testImageProvider() {
        QFETCH(QString, id);
        QFETCH(bool, async);
        QFETCH(QString, error);

        QString source = QStringLiteral("image://qcorotest/%1.jpg").arg(id);

        auto provider = new TestImageProvider(async, error);

        QQmlEngine engine;
        engine.addImageProvider(QStringLiteral("qcorotest"), provider);
        QVERIFY(engine.imageProvider(QStringLiteral("qcorotest")) != nullptr);

        const QString componentStr = QStringLiteral(R"(
import QtQuick 2.0

Image {
    source: "%1";
    asynchronous: %2;
})").arg(source, async ? QStringLiteral("true") : QStringLiteral("false"));

        QQmlComponent component(&engine);
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(QStringLiteral("")));

        std::unique_ptr<QQuickImage> image{qobject_cast<QQuickImage *>(component.create())};
        QVERIFY(image != nullptr);

        if (async) {
            QTRY_COMPARE(image->status(), QQuickImage::Loading);
        }

        QCOMPARE(image->source(), QUrl(source));

        if (error.isEmpty()) {
            QTRY_COMPARE(image->status(), QQuickImage::Ready);

            QCOMPARE(image->progress(), 1.0);
        } else {
            QTRY_COMPARE(image->status(), QQuickImage::Error);
        }
    }

private:
};

QTEST_MAIN(QCoroImageProviderTest)

#include "qcoroimageprovider.moc"

// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testlibs/testhttpserver.h"

#include "qcoro/webengine/qcorowebenginepage.h"

#include <QTest>
#include <QTcpServer>

#include <QWebEngineView>

class QCoroWebEnginePageTest : public QCoro::TestObject<QCoroWebEnginePageTest> {
    Q_OBJECT
private:
    #if QT_VERSION_MAJOR == 5
    using LoadingResult = bool;
    #else
    using LoadingResult = QWebEngineLoadingInfo;
    #endif

    QCoro::Task<LoadingResult> load(QWebEnginePage *page, const QString &path) {
        return qCoro(page).load(QUrl{QStringLiteral("http://%1:%2/%3")
            .arg(QHostAddress{QHostAddress::LocalHost}.toString())
            .arg(mServer.port())
            .arg(path)});
    }

    QCoro::Task<> testFindText_coro(QCoro::TestContext) {
        QWebEngineView view;
        co_await load(view.page(), QStringLiteral("stream"));

        qDebug() << (co_await qCoro(view.page()).toHtml());
        const auto findResult = co_await qCoro(view.page()).findText(QStringLiteral("Hola"));
        QCORO_COMPARE(findResult.numberOfMatches(), 10);
        QCORO_COMPARE(findResult.activeMatch(), 0);
    }

private Q_SLOTS:
    addTest(FindText)


    void init() {
        mServer.start(QHostAddress::LocalHost);
    }

    void cleanup() {
        mServer.stop();
    }

private:
    TestHttpServer<QTcpServer> mServer;
};

QTEST_MAIN(QCoroWebEnginePageTest)

#include "qcorowebenginepage.moc"
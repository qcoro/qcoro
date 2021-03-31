// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/network.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QHostAddress>
#include <QDebug>

#include <thread>
#include <memory>

class HttpServer {
public:
    void start() {
        assert(!mThread.joinable());

        mThread = std::thread([this]() { run(); });
        std::unique_lock lock(mReadyMutex);
        mServerReady.wait(lock);
    }

    void stop() {
        mThread.join();
    }

    uint16_t port() const {
        return mPort;
    }

private:
    void run() {
        QTcpServer server{};
        server.listen(QHostAddress::LocalHost);
        {
            std::unique_lock lock(mReadyMutex);
            mPort = server.serverPort();
            mServerReady.notify_all();
        }

        if (!server.waitForNewConnection(2000)) {
            return;
        }
        auto *conn = server.nextPendingConnection();
        if (conn->waitForReadyRead(1000)) {
            const auto request = conn->readLine();
            if (request == "GET /block HTTP/1.1\r\n") {
                std::this_thread::sleep_for(500ms);
            }
            conn->write("HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "abcdef");
            conn->flush();
            conn->close();
        }

        delete conn;
    }


    std::thread mThread;
    std::mutex mReadyMutex;
    std::condition_variable mServerReady;
    uint16_t mPort;
};


class QCoroNetworkReplyTest : public QCoro::TestObject<QCoroNetworkReplyTest> {
    Q_OBJECT
private:

    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://localhost:%1").arg(mServer.port())});

        co_await reply;

        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");

        delete reply;
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://localhost:%1/block").arg(mServer.port())});

        co_await reply;

        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");

        delete reply;
    }

    QCoro::Task<> testDoesntCoAwaitNullReply_coro(QCoro::TestContext test) {
        test.setShouldNotSuspend();

        QNetworkReply *reply = nullptr;

        co_await reply;

        delete reply;
    }

    QCoro::Task<> testDoesntCoAwaitFinishedReply_coro(QCoro::TestContext test) {
        QNetworkAccessManager nam;
        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://localhost:%1").arg(mServer.port())});

        co_await reply;

        QCORO_VERIFY(reply->isFinished());

        test.setShouldNotSuspend();
        co_await reply;

        delete reply;
    }

private Q_SLOTS:
    void init() {
        mServer.start();
    }

    void cleanup() {
        mServer.stop();
    }

    addTest(Triggers)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitNullReply)
    addTest(DoesntCoAwaitFinishedReply)

private:
    HttpServer mServer;
};

QTEST_GUILESS_MAIN(QCoroNetworkReplyTest)

#include "qnetworkreply.moc"

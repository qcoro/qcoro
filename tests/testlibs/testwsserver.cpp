// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testwsserver.h"

#include <QWebSocket>
#include <QWebSocketServer>
#include <QDebug>
#include <QTimer>
#include <QRandomGenerator>
#include <QTest>

#include <chrono>
#include <cstring>

using namespace std::chrono_literals;

class Server : public QObject {
    Q_OBJECT
public:
    explicit Server() = default;

    void setExpectTimeout() {
        mExpectTimeout = true;
    }

    QUrl waitForStart() {
        std::unique_lock lock(mMutex);
        mCond.wait(lock, [this]() { return !mUrl.isEmpty(); });
        return mUrl;
    }

    bool waitForConnection() {
        std::unique_lock lock(mMutex);
        if (!mSocket) {
            return mCond.wait_for(lock, 5s, [this]() -> bool { return mSocket.get() != nullptr; });
        }
        return true;

    }

    void start() {
        mServer.reset(new QWebSocketServer(QStringLiteral("QCoroTestWSServer"), QWebSocketServer::NonSecureMode));
        if (!mServer->listen(QHostAddress::LocalHost)) {
            qCritical() << "WebSocket server failed to start listening";
            close();
            return;
        }

        std::scoped_lock lock(mMutex);
        mUrl = mServer->serverUrl();

        mTimeout.reset(new QTimer());
        connect(mTimeout.get(), &QTimer::timeout, this, &Server::newConnectionTimeout);
        mTimeout->setSingleShot(true);
        mTimeout->start(10s);

        connect(mServer.get(), &QWebSocketServer::newConnection,
                this, &Server::newConnection);
        connect(mServer.get(), &QWebSocketServer::acceptError,
                this, [this](auto error) {
                    qCritical() << "WebSocket server failed to accept incoming connection:" << error;
                    close();
                });
        connect(mServer.get(), &QWebSocketServer::serverError,
                this, [this](auto error) {
                    qCritical() << "WebSocket server failed to set up WS connection" << error;
                    close();
                });

        mCond.notify_all();
    }

    void close() {
        QThread::currentThread()->quit();
        mSocket.reset();
        mTimeout.reset();
        mServer.reset();
    }
private Q_SLOTS:
    void newConnectionTimeout() {
        if (!mExpectTimeout) {
            QFAIL("No incoming connection within timeout");
        }
        close();
    }

    void newConnection() {
        mTimeout->stop();

        {
            std::scoped_lock lock(mMutex);
            mSocket.reset(mServer->nextPendingConnection());
        }
        mCond.notify_all();

        mTimeout.reset(new QTimer());
        connect(mTimeout.get(), &QTimer::timeout, this, [this]() {
            if (!mExpectTimeout) {
                QFAIL("No incoming request within timeout");
            }
            close();
        });
        mTimeout->setSingleShot(true);
        mTimeout->start(5s);

        connect(mSocket.get(), &QWebSocket::textMessageReceived,
                this, [this](const QString &msg) {
                    mTimeout->stop();
                    const auto request = mSocket->requestUrl().path();
                    if (request == QLatin1String("/delay")) {
                        std::this_thread::sleep_for(300ms);
                        mSocket->sendTextMessage(msg);
                    } else if (request == QLatin1String("/large")) {
                        const auto response = QString::fromLatin1(generateLargeMessage().toHex());
                        mSocket->sendTextMessage(response);
                    } else {
                        mSocket->sendTextMessage(msg);
                    }
                });
        connect(mSocket.get(), &QWebSocket::binaryMessageReceived,
                this, [this](const QByteArray &msg) {
                    mTimeout->stop();
                    const auto request = mSocket->requestUrl().path();
                    if (request == QLatin1String("/delay")) {
                        std::this_thread::sleep_for(100ms);
                        mSocket->sendBinaryMessage(msg);
                    } else if (request == QLatin1String("/large")) {
                        mSocket->sendBinaryMessage(generateLargeMessage());
                    } else {
                        mSocket->sendBinaryMessage(msg);
                    }
                });
    }

private:
    QByteArray generateLargeMessage() const {
        constexpr qsizetype size = 10 * 1024 * 1024; /* 10MiB */
        std::vector<uint64_t> buffer;
        buffer.resize(size);
        QRandomGenerator::global()->fillRange(buffer.data(), buffer.size());

        QByteArray msg;
        msg.resize(size);
        std::memcpy(msg.data(), buffer.data(), buffer.size());
        return msg;
    }

    std::unique_ptr<QWebSocket> mSocket;
    std::unique_ptr<QWebSocketServer> mServer;
    std::unique_ptr<QTimer> mTimeout;
    std::condition_variable mCond;
    std::mutex mMutex;
    std::atomic_bool mExpectTimeout;
    QUrl mUrl;
};

TestWsServer::TestWsServer() = default;
TestWsServer::~TestWsServer() = default;

void TestWsServer::start() {
    mThread.reset(new QThread());
    mThread->start();

    mServer.reset(new Server());
    mServer->moveToThread(mThread.get());

    QMetaObject::invokeMethod(mServer.get(), &Server::start, Qt::QueuedConnection);

    mUrl = mServer->waitForStart();
}

void TestWsServer::stop() {
    if (mThread && mThread->isRunning()) {
        QMetaObject::invokeMethod(mServer.get(), &Server::close, Qt::BlockingQueuedConnection);
        mThread->wait();
    }
    mThread.reset();
    mServer.reset();
    mUrl.clear();
}

QUrl TestWsServer::url() const {
    return mUrl;
}

void TestWsServer::setExpectTimeout() {
    mServer->setExpectTimeout();
}

bool TestWsServer::waitForConnection() {
    return mServer->waitForConnection();
}

#include "testwsserver.moc"

// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QThread>
#include <QDebug>

#include <mutex>
#include <condition_variable>

class QTcpServer;
class QLocalServer;

template<typename Func>
class Thread : public QThread
{
public:
    explicit Thread(Func &&f)
        : mFunc(std::forward<Func>(f))
    {}

    ~Thread() = default;

    void run() {
        mFunc();
    }

private:
    Func mFunc;
};

template<typename Func>
Thread(Func &&) -> Thread<Func>;

template<typename ServerType>
class TestHttpServer {
public:
    template<typename T>
    void start(const T &name) {
        // Can't use QThread::create, it's only available when Qt is built with C++17,
        // which some distros don't have :(
        mThread.reset(new Thread([this, name]() { run(name); }));
        mThread->start();
        std::unique_lock lock(mReadyMutex);
        mServerReady.wait(lock, [this]() { return mPort != 0; });
    }

    void stop() {
        mThread->wait();
        mThread.reset();
        mPort = 0;
    }

    uint16_t port() const {
        return mPort;
    }

private:
    template<typename T>
    void run(const T &name) {
        using namespace std::chrono_literals;

        ServerType server{};
        if (!server.listen(name)) {
            qDebug() << "Error listening:" << server.serverError();
            return;
        }
        assert(server.isListening());

        {
            std::scoped_lock lock(mReadyMutex);
            if constexpr (std::is_same_v<ServerType, QTcpServer>) {
                mPort = server.serverPort();
            } else {
                mPort = 1;
            }
        }

        mServerReady.notify_all();

        if (!server.waitForNewConnection(2000)) {
            mPort = 0;
            return;
        }

        auto *conn = server.nextPendingConnection();
        if (conn->waitForReadyRead(1000)) {
            const auto request = conn->readLine();
            if (request == "GET /stream HTTP/1.1\r\n") {
                QStringList lines;
                for (int i = 0; i < 10; ++i) {
                    lines.push_back(QStringLiteral("Hola %1\n").arg(i));
                }

                const auto len = std::accumulate(lines.cbegin(), lines.cend(), 0, [](int l, const QString &s) { return l + s.size(); });
                conn->write("HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " + QByteArray::number(len) + "\r\n"
                            "\r\n");
                conn->flush();
                for (const auto &line : lines) {
                    conn->write(line.toUtf8());
                    conn->flush();
                    std::this_thread::sleep_for(100ms);
                }
            } else {
                if (request == "GET /block HTTP/1.1\r\n") {
                    std::this_thread::sleep_for(500ms);
                }

                conn->write("HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "\r\n"
                            "abcdef");
            }
            conn->flush();
            conn->close();
        }

        delete conn;
        mPort = 0;
    }


    std::unique_ptr<QThread> mThread;
    std::mutex mReadyMutex;
    std::condition_variable mServerReady;
    uint16_t mPort = 0;
};



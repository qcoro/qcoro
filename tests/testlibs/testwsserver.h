// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QThread>
#include <QUrl>

#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

class Server;
class TestWsServer {
public:
    TestWsServer();
    ~TestWsServer();

    void start();
    void stop();
    QUrl url() const;
    bool waitForConnection();

    void setExpectTimeout();
private:
    std::unique_ptr<QThread> mThread;
    std::unique_ptr<Server> mServer;
    QUrl mUrl;
};

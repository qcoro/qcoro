// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorotask.h"
#include "qcorotimer.h"

#include <QCoreApplication>
#include <QTimer>

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

QCoro::Task<QString> generateRandomString() {
    std::cout << "GenerateRandomString started" << std::endl;
    QTimer timer;
    timer.start(1s);
    std::cout << "GenerateRandomString \"generating\"..." << std::endl;
    co_await timer;
    std::cout << "GenerateRandomString finished \"generating\"" << std::endl;

    std::cout << "GenerateRandomString co_returning to caller" << std::endl;
    co_return QStringLiteral("RandomString!");
}

QCoro::Task<qsizetype> generateRandomNumber() {
    std::cout << "GenerateRandomNumber started" << std::endl;
    std::cout << "GenerateRandomNumber co_awaiting on generateRandomString()" << std::endl;
    const QString string = co_await generateRandomString();
    std::cout << "GenerateRandomNumber successfully co_awaited on generateRandomString() and "
                 "co_returns result"
              << std::endl;
    co_return string.size();
}

QCoro::Task<> logRandomNumber() {
    std::cout << "LogRandomNumber started" << std::endl;
    std::cout << "LogRandomNumber co_awaiting on generateRandomNumber()" << std::endl;
    const int number = co_await generateRandomNumber();
    std::cout << "Random number for today is: " << number << std::endl;

    qApp->quit();
}

int main(int argc, char **argv) {
    QCoreApplication app{argc, argv};
    QTimer::singleShot(0, qApp, logRandomNumber);
    return app.exec();
}

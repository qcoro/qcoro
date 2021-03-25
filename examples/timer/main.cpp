// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoro/task.h"
#include "qcoro/timer.h"

#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>

#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

QCoro::Task<> runMainTimer()
{
    QTimer timer;
    timer.setInterval(1s);
    timer.start();

    std::cout << "Waiting for main timer..." << std::endl;
    co_await timer;
    std::cout << "Main timer ticked!" << std::endl;

    qApp->quit();
}

int main(int argc, char **argv)
{
    QCoreApplication app{argc, argv};
    QTimer ticker;
    QObject::connect(&ticker, &QTimer::timeout,
                     &app, []() {
                        std::cout << QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString()
                                  << " Secondary timer tick!" << std::endl;
                     });
    ticker.start(200ms);

    QTimer::singleShot(0, runMainTimer);
    return app.exec();
}

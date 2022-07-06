#include "testloop.h"

#include <QTest>

TestLoop::TestLoop(QObject *parent)
    : QObject(parent)
{
    mTimer.setSingleShot(true);
    connect(&mTimer, &QTimer::timeout, this, [this]() {
        mEventLoop.quit();
        QFAIL("Test timeout!");
    });
}

void TestLoop::exec() {
    mEventLoop.exec();
}

void TestLoop::quit() {
    mTimer.stop();
    QTimer::singleShot(0, &mEventLoop, &QEventLoop::quit);
}

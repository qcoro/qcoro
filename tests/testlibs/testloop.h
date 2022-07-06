#pragma once

#include <QEventLoop>
#include <QTimer>

#define DELAYED(expr) \
    QTimer::singleShot(10ms, [&]() { expr; })

class TestLoop : public QObject {
    Q_OBJECT
public:
    explicit TestLoop(QObject *parent = nullptr);

    void exec();
    void quit();

private:
    QEventLoop mEventLoop;
    QTimer mTimer;
};

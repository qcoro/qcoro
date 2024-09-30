#include <QCoreApplication>
#include <QFile>

#include "qcorotask.h"
#include "qcorotimer.h"
#include "qcoroiodevice.h"

using namespace std::chrono_literals;

// We could use std::stop_token, but it is not supported by AppleClang
struct Stop {
public:
    void requestStop() {
        mShouldStop = true;
    }

    bool stopRequested() const {
        return mShouldStop;
    }
private:
    bool mShouldStop = false;
};

auto backgroundTask(const Stop &stop) -> QCoro::Task<> {
    qDebug() << "Task: Background task started, waiting for event loop";
    co_await QCoro::sleepFor(0ms);
    qDebug() << "Task: Event loop is running";
    QFile file(QStringLiteral("/dev/stdin"));
    file.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    while (!stop.stopRequested()) {
        qDebug() << "Task: Waiting for input...";
        const auto result = co_await qCoro(file).readLine(1024, 5s);
        if (!result.isEmpty()) {
            qDebug() << "Task: Read line:" << result;
        } else {
            qDebug() << "Task: Timeout!";
        }
    }
    qDebug() << "Task: Backround task stopped";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Stop stop;
    auto bgTask = backgroundTask(stop);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&stop] {
        qDebug() << "App: Requesting background task to stop";
        stop.requestStop();
    });
    QTimer::singleShot(500ms, &app, []() {
        qDebug() << "App: Stopping application";
        QCoreApplication::instance()->quit();
    });

    // Run the app
    qDebug() << "App: Starting application event loop";
    const auto result = app.exec();
    qDebug() << "App: Application event loop stopped";

    // Wait for the background task to complete
    qDebug() << "App: Waiting for background task to complete";
    QCoro::waitFor(bgTask);
    qDebug() << "App: Background task completed";

    return result;
}
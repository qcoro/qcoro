#include <QCoroThread>
#include <QThread>

#include <memory>

QCoro::Task<void> MainWindow::processData(const QVector<qint64> &data) {
    std::unique_ptr<QThread> thread(QThread::create([data]() {
        // Perform some intesive calculation
    }));
    thread->start();

    ui->setState(tr("Processing is starting..."));
    co_await qCoro(thread.get()).waitForStarted();
    ui->setState(tr("Processing data..."));
    co_await qCoro(thread.get()).waitForFinished();
    ui->setState(tr("Processing done."));
}

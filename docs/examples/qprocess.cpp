#include <QCoroProcess>

QCoro::Task<QByteArray> listDir(const QString &dirPath) {
    QProcess basicProcess;
    auto process = qCoro(basicProcess);
    qDebug() << "Starting ls...";
    co_await process.start(QStringLiteral("/bin/ls"), {dirPath});
    qDebug() << "Ls started, reading directory...";

    co_await process.waitForFinished();
    qDebug() << "Done";

    return basicProcess.readAll();
}


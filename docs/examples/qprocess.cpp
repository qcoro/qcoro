#include <QCoroProcess>

QCoro::Task<QStringList> listDir(const QString &dirPath) {
    QProcess process;
    qDebug() << "Starting ls...";
    co_await process.start(QStringLiteral("/bin/ls"), {dirPath});
    qDebug() << "Ls started, reading directory...";

    co_await process.waitForFinished();
    qDebug() << "Done";

    return process.readAll();
}


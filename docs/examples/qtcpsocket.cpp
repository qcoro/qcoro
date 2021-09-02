#include <QCoroTcpSocket>

QCoro::Task<QByteArray> requestDataFromServer(const QString &hostName) {
    QTcpSocket socket;
    if (!co_await qCoro(socket).connectToHost(hostName)) {
        qWarning() << "Failed to connect to the server";
        co_return QByteArray{};
    }

    socket.write("SEND ME DATA!");

    QByteArray data;
    while (!data.endsWith("\r\n.\r\n")) {
        data += co_await qCoro(socket).readAll();
    }

    co_return data;
}
`

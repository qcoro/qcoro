#include <QCoroTcpServer>

QCoro::Task<> runServer(uint16_t port) {
    QTcpServer server;
    server.listen(QHostAddress::LocalHost, port);

    while (server.isListening()) {
        auto *socket = co_await qCoro(server).waitForNewConnection(10s);
        if (socket != nullptr) {
            newClientConnection(socket);
        }
    }
}


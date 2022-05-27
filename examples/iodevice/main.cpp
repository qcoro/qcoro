#include "qcoroiodevice.h"
#include "qcorotask.h"

#include <QCoreApplication>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

class Server : public QObject {
    Q_OBJECT
public:
    explicit Server(QHostAddress addr, uint16_t port) {
        mServer.listen(addr, port);
        connect(&mServer, &QTcpServer::newConnection, this, &Server::handleConnection);
    }

private Q_SLOTS:
    QCoro::Task<> handleConnection() {
        auto socket = mServer.nextPendingConnection();
        while (socket->isOpen()) {
            const auto data = co_await socket;
            socket->write("PONG: " + data);
        }
    }

private:
    QTcpServer mServer;
};

class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(QHostAddress addr, uint16_t port) {
        mSocket.connectToHost(addr, port);
        connect(&mTimer, &QTimer::timeout, this, &Client::sendPing);
        mTimer.start(300ms);
    }

private Q_SLOTS:
    QCoro::Task<> sendPing() {
        std::cout << "Sending ping..." << std::endl;
        mSocket.write(QByteArray("PING #") + QByteArray::number(++mPing));
        const auto response = co_await mSocket;
        std::cout << "Received pong: " << response.constData() << std::endl;
    }

private:
    int mPing = 0;
    QTcpSocket mSocket;
    QTimer mTimer;
};

int main(int argc, char **argv) {
    QCoreApplication app{argc, argv};
    Server server{QHostAddress::LocalHost, 6666};
    Client client{QHostAddress::LocalHost, 6666};
    return app.exec();
}

#include "main.moc"

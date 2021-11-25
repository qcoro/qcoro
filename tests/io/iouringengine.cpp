#include "../../qcoro/io/ioengine_p.h"

#include "../testobject.h"

#include <QTest>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QTemporaryFile>
#include <QCoreApplication>
#include <QScopeGuard>
#include <QRandomGenerator>

#include <unistd.h>
#include <fcntl.h>

static const char file_content[] =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla imperdiet dapibus mi a blandit.\n"
    "In hac habitasse platea dictumst. Proin ut metus a odio faucibus eleifend. Nulla non rhoncus leo. "
    "Etiam congue nisl eget lectus gravida, in imperdiet est suscipit. Nunc quis ullamcorper turpis.\n"
    "Mauris blandit leo ligula. Nam congue sem id quam eleifend, sit amet ullamcorper elit tempor. "
    "Sed ligula nisi, sagittis sit amet sollicitudin sit amet, interdum quis ligula. Nulla facilisis "
    "dui sit amet pellentesque aliquet. Proin imperdiet eros tellus, at scelerisque nisl ultricies "
    "ac. Quisque ultrices sed mi sit amet lacinia. Sed at libero efficitur, pharetra turpis in, "
    "scelerisque turpis.\n"
    "Fusce sit amet tristique dui. Praesent eget odio a nibh rhoncus pharetra non sed ligula.\n";

class IOUringEngineTest : public QCoro::TestObject<IOUringEngineTest> {
    Q_OBJECT

private:
    QCoro::Task<> testOpenWriteOnly_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        const auto file = QDir::tempPath() + QStringLiteral("/%1-%2-testOpenWriteOnly")
                            .arg(qApp->applicationName()).arg(qApp->applicationPid());
        const auto fd = co_await engine.open(file, QCoroIO::FileMode::WriteOnly);
        QCORO_VERIFY(fd.has_value());
        QScopeGuard fdGuard([&]() {
            ::close(*fd);
            QFile::remove(file);
        });

        const int size = ::write(*fd, "Foobar", 6);
        QCORO_COMPARE(size, 6);
    }

    QCoro::Task<> testOpenReadOnly_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        QTemporaryFile tmpFile; // will take care of deleting the file
        QCORO_VERIFY(tmpFile.open());
        tmpFile.write(file_content);
        tmpFile.close();

        const auto fd = co_await engine.open(tmpFile.fileName(), QCoroIO::FileMode::ReadOnly);
        QCORO_VERIFY(fd.has_value());
        QScopeGuard fdGuard([fd]() {::close(*fd); });

        QByteArray buf;
        buf.resize(sizeof(file_content) - 1);
        const int size = ::read(*fd, buf.data(), buf.size());
        QCORO_COMPARE(size, buf.size());

        QCORO_COMPARE(QByteArray{file_content}, buf);
    }

    QCoro::Task<> testOpenReadWrite_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        const auto file = QDir::tempPath() + QStringLiteral("/%1-%2-testOpenReadWrite")
                            .arg(qApp->applicationName()).arg(qApp->applicationPid());
        const auto fd = co_await engine.open(file, QCoroIO::FileMode::ReadWrite);
        QCORO_VERIFY(fd.has_value());
        QScopeGuard fdGuard([&]() {
                ::close(*fd);
                QFile::remove(file);
        });

        {
            const int size = ::write(*fd, file_content, sizeof(file_content) - 1);
            QCORO_COMPARE(size, sizeof(file_content) - 1);
        }

        lseek(*fd, 0, SEEK_SET);

        QByteArray buf;
        buf.resize(sizeof(file_content) - 1);
        {
            const int size = ::read(*fd, buf.data(), buf.size());
            QCORO_COMPARE(size, buf.size());
        }

        QCORO_COMPARE(QByteArray{file_content}, buf);
    }

    QCoro::Task<> testOpenError_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        const auto file = QDir::tempPath() + QStringLiteral("/%1-%2-nonexistentfile")
                            .arg(qApp->applicationName()).arg(qApp->applicationPid());
        const auto fd = co_await engine.open(file, QCoroIO::FileMode::ReadOnly | QCoroIO::FileMode::ExistingOnly);
        QCORO_VERIFY(!fd.has_value());
        QCORO_COMPARE(fd.error().value(), ENOENT); 
    }

    QCoro::Task<> testClose_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        const auto file = QDir::tempPath() + QStringLiteral("/%1-%2-testClose")
                            .arg(qApp->applicationName()).arg(qApp->applicationPid());
        const int fd = ::open(qUtf8Printable(file), O_RDONLY | O_CREAT);
        QCORO_VERIFY(fd > 0);
        QScopeGuard fdGuard([&]() {
            QFile::remove(file);
        });

        const auto result = co_await engine.close(fd);
        QCORO_VERIFY(static_cast<bool>(result));
    }

    QCoro::Task<> testCloseError_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        int fd = 0;
        for (int i = 100; i < 10000; ++i) {
            if (!QFile::exists(QStringLiteral("/proc/self/%1").arg(i))) {
                fd = i;
                break;
            }
        }
        if (fd == 0) {
            QCORO_FAIL("Failed to find free FD");
        }

        const auto result = co_await engine.close(fd);
        QCORO_VERIFY(!result.has_value());
        QCORO_COMPARE(result.error().value(), EBADF);
    }

    QCoro::Task<> testRead_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        QTemporaryFile tmpFile;
        QCORO_VERIFY(tmpFile.open());
        tmpFile.write(file_content);
        tmpFile.close();

        QFile readFile(tmpFile.fileName());
        QCORO_VERIFY(readFile.open(QIODevice::ReadOnly));

        const size_t fileSize = readFile.size();
        QByteArray data;
        while (static_cast<size_t>(data.size()) < fileSize) {
            const auto newData = co_await engine.read(readFile.handle(), 10, data.size());
            if (newData) {
                data += *newData;
            } else {
                qWarning() << "Read failed: " << newData.error();
                QCORO_FAIL("Read failed");
            }
        }

        QCORO_COMPARE(data, QByteArray{file_content});
    }

    QCoro::Task<> testWrite_coro(QCoro::TestContext) {
        QCoroIO::IOEngine engine;
        QCORO_VERIFY(engine.init());

        QTemporaryFile tmpFile;
        QCORO_VERIFY(tmpFile.open());
        for (size_t i = 0; i < sizeof(file_content) - 1; ) {
            const auto to_write = std::min(sizeof(file_content) - 1 - i, 10UL);
            const auto written = co_await engine.write(tmpFile.handle(), file_content + i, to_write, i);
            if (written) {
                i += *written;
            } else {
                qWarning() << "Write failed: " << written.error();
                QCORO_FAIL("Write failed")
            }
        }

        tmpFile.close();

        tmpFile.open();
        const auto fileContent = tmpFile.readAll();
        QCORO_COMPARE(fileContent.size(), sizeof(file_content) - 1);
        QCORO_COMPARE(fileContent, QByteArray{file_content});
    }

private Q_SLOTS:
    addTest(OpenWriteOnly)
    addTest(OpenReadOnly)
    addTest(OpenReadWrite)
    addTest(OpenError)
    addTest(Close)
    addTest(CloseError)
    addTest(Read)
    addTest(Write)

};

QTEST_GUILESS_MAIN(IOUringEngineTest)

#include "iouringengine.moc"

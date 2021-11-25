#include "../../qcoro/io/ioengine_p.h"

#include "../testobject.h"

#include <QTest>
#include <QObject>
#include <QFile>
#include <QTemporaryFile>

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
    addTest(Read)
    addTest(Write)

};

QTEST_GUILESS_MAIN(IOUringEngineTest)

#include "iouringengine.moc"

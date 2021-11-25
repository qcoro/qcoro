#pragma once

#include "coroutine.h"
#include "expected.h"
#include "qcoroio.h"

#include <QObject>
#include <QByteArray>

namespace QCoroIO {

class OpenOperation;
class CloseOperation;
class WriteOperation;

class ReadOperationPrivate;
class ReadOperation  {
public:
    ReadOperation(ReadOperation &&) noexcept = default;
    ReadOperation &operator=(ReadOperation &&) noexcept = default;
    ~ReadOperation();

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QCoro::Expected<QByteArray, QCoro::ErrorCode> await_resume() const noexcept;

private:
    explicit ReadOperation(std::unique_ptr<ReadOperationPrivate> dd);

    std::unique_ptr<ReadOperationPrivate> d;

    friend class IOEngine;
};

class WriteOperationPrivate;
class WriteOperation {
public:
    WriteOperation(WriteOperation &&) noexcept = default;
    WriteOperation &operator=(WriteOperation &&) noexcept = default;
    ~WriteOperation();

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QCoro::Expected<ssize_t, QCoro::ErrorCode> await_resume() const noexcept;

private:
    explicit WriteOperation(std::unique_ptr<WriteOperationPrivate> dd);

    std::unique_ptr<WriteOperationPrivate> d;

    friend class IOEngine;
};

class IOEnginePrivate;
class IOEngine : public QObject {
    Q_OBJECT
public:
    explicit IOEngine();
    ~IOEngine();

    bool init();

    //virtual OpenOperation *open(int fd, FileMode mode) = 0;
    //virtual CloseOperation *close() = 0;

    ReadOperation read(int fd, std::size_t size, std::size_t offset);
    WriteOperation write(int fd, const void *data, std::size_t dataLen, std::size_t offset);

private:
    const std::unique_ptr<IOEnginePrivate> d;
};

} // namespace QCoroIO

#pragma once

#include "coroutine.h"
#include "expected.h"
#include "qcoroio.h"

#include <QObject>
#include <QByteArray>

namespace QCoroIO {

class OpenOperationPrivate;
class OpenOperation {
public:
    OpenOperation(OpenOperation &&) noexcept = default;
    OpenOperation &operator=(OpenOperation &&) noexcept = default;
    ~OpenOperation();

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QCoro::Expected<int> await_resume() const noexcept;

private:
    explicit OpenOperation(std::unique_ptr<OpenOperationPrivate> dd);

    std::unique_ptr<OpenOperationPrivate> d;

    friend class IOEngine;
};

class CloseOperationPrivate;
class CloseOperation {
public:
    CloseOperation(CloseOperation &&) noexcept = default;
    CloseOperation &operator=(CloseOperation &&) noexcept = default;
    ~CloseOperation();

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QCoro::Expected<void> await_resume() const noexcept;

private:
    explicit CloseOperation(std::unique_ptr<CloseOperationPrivate> dd);

    std::unique_ptr<CloseOperationPrivate> d;

    friend class IOEngine;
};

class ReadOperationPrivate;
class ReadOperation  {
public:
    ReadOperation(ReadOperation &&) noexcept = default;
    ReadOperation &operator=(ReadOperation &&) noexcept = default;
    ~ReadOperation();

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QCoro::Expected<QByteArray> await_resume() const noexcept;

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
    QCoro::Expected<ssize_t> await_resume() const noexcept;

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

    OpenOperation open(const QString &path, FileModes mode);
    CloseOperation close(int fd);

    ReadOperation read(int fd, std::size_t size, std::size_t offset);
    WriteOperation write(int fd, const void *data, std::size_t dataLen, std::size_t offset);

private:
    const std::unique_ptr<IOEnginePrivate> d;
};

} // namespace QCoroIO

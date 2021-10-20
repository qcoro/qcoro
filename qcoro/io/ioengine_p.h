#pragma once

#include "coroutine.h"
#include "qcoroio.h"

#include <QObject>
#include <QByteArray>

namespace QCoroIO {

class OpenOperation;
class CloseOperation;
class WriteOperation;

class ReadOperationImpl {
public:
    virtual ~ReadOperationImpl() = default;
    virtual bool awaitReady() const noexcept = 0;
    virtual void awaitSuspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept = 0;
    virtual QByteArray awaitResume() = 0;

protected:
    explicit ReadOperationImpl() = default;
};

class ReadOperation  {
public:
    explicit ReadOperation(std::unique_ptr<ReadOperationImpl> impl);
    ReadOperation(ReadOperation &&) noexcept = default;
    ReadOperation &operator=(ReadOperation &&) noexcept = default;

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QByteArray await_resume();

private:
    std::unique_ptr<ReadOperationImpl> d;
};

class IOEngine : public QObject {
    Q_OBJECT
public:
    virtual bool init() = 0;

    //virtual OpenOperation *open(int fd, FileMode mode) = 0;
    //virtual CloseOperation *close() = 0;

    virtual ReadOperation read(int fd, size_t size, size_t offset) = 0;
    //virtual WriteOperation *write(const uint8_t *data, size_t data_len) = 0;
};

} // namespace QCoroIO

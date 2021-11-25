#include "ioengine_p.h"

#include <liburing.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <system_error>

#include <QDebug>
#include <QSocketNotifier>
#include <QTimer>

using namespace QCoroIO;

static constexpr int ringEntryCount = 8;
static constexpr std::size_t readBufSize = 4096;

namespace {

class IOUringOperation {
public:
    virtual void complete(int32_t size) = 0;
};

} // namespace

namespace QCoroIO {

class IOEnginePrivate {
public:
    void handleEventFd() {
        eventfd_t event = {};
        eventfd_read(evfd, &event);

        struct io_uring_cqe *cqe = {};

        if (const int ret = io_uring_wait_cqe(&ring, &cqe); ret < 0) {
            qWarning("Failed to wait for io_uring CQE: %s (%d)", strerror(-ret), -ret);
            return;
        }

        IOUringOperation *op = static_cast<IOUringOperation *>(io_uring_cqe_get_data(cqe));
        Q_ASSERT(op != nullptr);
        op->complete(cqe->res);

        io_uring_cqe_seen(&ring, cqe);
    }

public:
    struct io_uring ring;
    std::unique_ptr<QSocketNotifier> socketNotifier;
    int evfd = 0;
};

class OpenOperationPrivate final : public IOUringOperation {
public:
    OpenOperationPrivate(struct io_uring *ring, const QString &filePath, QCoroIO::FileModes fileMode) {
        auto *sqe = io_uring_get_sqe(ring);
        int flags = O_RDONLY;
        if (static_cast<int>(fileMode & FileMode::ReadWrite) == static_cast<int>(FileMode::ReadWrite)) {
            flags = O_RDWR;
        } else if (fileMode & FileMode::WriteOnly) {
            flags = O_WRONLY;
        }

        if ((fileMode & FileMode::WriteOnly) && !(fileMode & FileMode::ExistingOnly)) {
            flags |= O_CREAT;
        }
        if (fileMode & QCoroIO::FileMode::Truncate) {
            flags |= O_TRUNC;
        }
        if (fileMode & QCoroIO::FileMode::Append) {
            flags |= O_APPEND;
        }
        if (fileMode & QCoroIO::FileMode::NewOnly) {
            flags &= ~O_EXCL;
        }

        io_uring_prep_openat(sqe, 0, qUtf8Printable(filePath), flags, 0666);
        io_uring_sqe_set_data(sqe, this);
        io_uring_submit(ring);
    }

    void complete(int32_t fd) override {
        if (fd < 1) {
            m_res = QCoro::makeUnexpected(QCoro::ErrorCode(-fd, std::system_category()));
        } else {
            m_res = static_cast<int>(fd);
        }

        m_complete = true;
        QTimer::singleShot(0, [coroutine = m_awaitingCoroutine]() mutable {
            coroutine.resume();
        });
    }

public:
    bool m_complete = false;
    QCORO_STD::coroutine_handle<> m_awaitingCoroutine;
    QCoro::Expected<int> m_res;
};

class CloseOperationPrivate final : public IOUringOperation {
public:
    CloseOperationPrivate(struct io_uring *ring, int fd) {
        auto *sqe = io_uring_get_sqe(ring);
        io_uring_prep_close(sqe, fd);
        io_uring_sqe_set_data(sqe, this);
        io_uring_submit(ring);
    }

    void complete(int32_t result) override {
        if (result < 0) {
            m_res = QCoro::makeUnexpected(QCoro::ErrorCode(-result, std::system_category()));
        } else {
            m_res = {};
        }

        m_complete = true;
        QTimer::singleShot(0, [coroutine = m_awaitingCoroutine]() mutable {
            coroutine.resume();
        });
    }

public:
    bool m_complete = false;
    QCORO_STD::coroutine_handle<> m_awaitingCoroutine;
    QCoro::Expected<void> m_res;
};

class ReadOperationPrivate final : public IOUringOperation {
public:
    ReadOperationPrivate(struct io_uring *ring, int fd, std::size_t size, std::size_t offset) {
        m_res = QByteArray();
        m_res->resize(std::min(size, readBufSize));
        auto *sqe = io_uring_get_sqe(ring);
        io_uring_prep_read(sqe, fd, m_res->data(), m_res->size(), offset);
        io_uring_sqe_set_data(sqe, this);
        io_uring_submit(ring);
    }

    void complete(int32_t size) override {
        if (size < 0) {
            m_res = QCoro::makeUnexpected(QCoro::ErrorCode(-size, std::system_category()));
        } else if (size < m_res->size()) {
            m_res->resize(size);
        }

        m_complete = true;
        QTimer::singleShot(0, [coroutine = m_awaitingCoroutine]() mutable {
            coroutine.resume();
        });
    }

public:
    bool m_complete = false;
    QCORO_STD::coroutine_handle<> m_awaitingCoroutine;
    QCoro::Expected<QByteArray> m_res;
};

class WriteOperationPrivate final : public IOUringOperation {
public:
    WriteOperationPrivate(struct io_uring *ring, int fd, const void *data, std::size_t dataLen, std::size_t offset) {
        auto *sqe = io_uring_get_sqe(ring);
        io_uring_prep_write(sqe, fd, data, dataLen, offset);
        io_uring_sqe_set_data(sqe, this);
        io_uring_submit(ring);
    }

    void complete(int32_t size) override {
        if (size < 0) {
            m_res = QCoro::makeUnexpected(QCoro::ErrorCode(-size, std::system_category()));
        } else {
            m_res = size;
        }

        m_complete = true;
        QTimer::singleShot(0, [coroutine = m_awaitingCoroutine]() mutable {
            coroutine.resume();
        });
    }

public:
    bool m_complete = false;
    QCORO_STD::coroutine_handle<> m_awaitingCoroutine;
    QCoro::Expected<ssize_t> m_res{0};
};

} // namespace QCoroIO


OpenOperation::OpenOperation(std::unique_ptr<OpenOperationPrivate> dd)
    : d(std::move(dd))
{}

OpenOperation::~OpenOperation() = default;

bool OpenOperation::await_ready() const noexcept {
    return d->m_complete;
}

void OpenOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    d->m_awaitingCoroutine = awaitingCoroutine;
}

QCoro::Expected<int> OpenOperation::await_resume() const noexcept {
    return d->m_res;
}


CloseOperation::CloseOperation(std::unique_ptr<CloseOperationPrivate> dd)
    : d(std::move(dd))
{}

CloseOperation::~CloseOperation() = default;

bool CloseOperation::await_ready() const noexcept {
    return d->m_complete;
}

void CloseOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    d->m_awaitingCoroutine = awaitingCoroutine;
}

QCoro::Expected<void> CloseOperation::await_resume() const noexcept {
    return d->m_res;
}


ReadOperation::ReadOperation(std::unique_ptr<ReadOperationPrivate> dd)
    : d(std::move(dd))
{}

ReadOperation::~ReadOperation() = default;

bool ReadOperation::await_ready() const noexcept {
    return d->m_complete;
}

void ReadOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    d->m_awaitingCoroutine = awaitingCoroutine;
}

QCoro::Expected<QByteArray> ReadOperation::await_resume() const noexcept {
    return d->m_res;
}


WriteOperation::WriteOperation(std::unique_ptr<WriteOperationPrivate> dd)
    : d(std::move(dd))
{}

WriteOperation::~WriteOperation() = default;

bool WriteOperation::await_ready() const noexcept {
    return d->m_complete;
}

void WriteOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    d->m_awaitingCoroutine = awaitingCoroutine;
}

QCoro::Expected<ssize_t> WriteOperation::await_resume() const noexcept {
    return d->m_res;
}


IOEngine::IOEngine():
    d(std::make_unique<IOEnginePrivate>())
{}

IOEngine::~IOEngine() {
    io_uring_unregister_eventfd(&d->ring);

    if (d->evfd > 0) {
        ::close(d->evfd);
        d->evfd = 0;
    }

    io_uring_queue_exit(&d->ring);
}

bool IOEngine::init() {
    d->evfd = ::eventfd(0, 0);
    if (d->evfd < 0) {
        const auto err = QCoro::ErrorCode(errno, std::system_category());
        qWarning() << "Failed to create eventfd object: " << err;
        return false;
    }

    d->socketNotifier = std::make_unique<QSocketNotifier>(d->evfd, QSocketNotifier::Read);
    connect(d->socketNotifier.get(), &QSocketNotifier::activated, this, [this]() { d->handleEventFd(); });
    d->socketNotifier->setEnabled(true);


    if (const int ret = io_uring_queue_init(ringEntryCount, &d->ring, /*flags=*/ 0); ret < 0) {
        const auto err = QCoro::ErrorCode(-ret, std::system_category());
        qWarning() << "Failed to create io_uring queue: " << err;
        ::close(d->evfd);
        d->evfd = 0;
        return false;
    }

    if (const int ret = io_uring_register_eventfd(&d->ring, d->evfd); ret < 0) {
        const auto err = QCoro::ErrorCode(-ret, std::system_category());
        qWarning() << "Failed to register eventfd for io_uring queue: " << err;
        ::close(d->evfd);
        d->evfd = 0;
        return false;
    }

    return true;
}

QCoroIO::OpenOperation IOEngine::open(const QString &path, FileModes mode) {
    return OpenOperation(std::make_unique<OpenOperationPrivate>(&(d->ring), path, mode));
}

QCoroIO::CloseOperation IOEngine::close(int fd) {
    return CloseOperation(std::make_unique<CloseOperationPrivate>(&(d->ring), fd));
}

QCoroIO::ReadOperation IOEngine::read(int fd, std::size_t size, std::size_t offset) {
    return ReadOperation(std::make_unique<ReadOperationPrivate>(&(d->ring), fd, size, offset));
}

QCoroIO::WriteOperation IOEngine::write(int fd, const void *data, std::size_t dataLen, std::size_t offset) {
    return WriteOperation(std::make_unique<WriteOperationPrivate>(&(d->ring), fd, data, dataLen, offset));

}


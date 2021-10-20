#include "iouring_engine.h"

#include <liburing.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <system_error>

#include <QDebug>
#include <QSocketNotifier>
#include <QTimer>

using namespace QCoroIO::Backend;

static constexpr int ringEntryCount = 8;
static constexpr size_t readBufSize = 4096;

class IOUringOperation {
public:
    virtual void complete(int32_t size) = 0;
};

class IOUringReadOperation : public IOUringOperation
                           , public QCoroIO::ReadOperationImpl {
public:
    IOUringReadOperation(struct io_uring *ring, int fd, size_t size, size_t offset) {
        m_buf.resize(std::min(size, readBufSize));
        auto *sqe = io_uring_get_sqe(ring);
        io_uring_prep_read(sqe, fd, m_buf.data(), m_buf.size(), offset);
        io_uring_sqe_set_data(sqe, this);
        io_uring_submit(ring);
    }

    ~IOUringReadOperation() override = default;

    bool awaitReady() const noexcept override {
        return m_complete;
    }

    void awaitSuspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept override {
        m_awaitingCoroutine = awaitingCoroutine;
    }

    QByteArray awaitResume() noexcept override {
        return m_buf;
    }

    void complete(int32_t size) override {
        if (size < m_buf.size()) {
            m_buf.resize(size);
        }
        m_complete = true;

        // HACK: we cannot resume the coroutine directly - it could cause the
        // resumed coroutine to destroy the IOUringEngine, causing crash in
        // handleEventFd() when we return from here.
        QTimer::singleShot(0, [coroutine = m_awaitingCoroutine]() mutable {
            coroutine.resume();
        });
    }

private:
    bool m_complete = false;
    QCORO_STD::coroutine_handle<> m_awaitingCoroutine;
    QByteArray m_buf;
};

IOUringEngine::IOUringEngine() {
}

IOUringEngine::~IOUringEngine() {
    io_uring_unregister_eventfd(&m_ring);

    if (m_evfd > 0) {
        ::close(m_evfd);
        m_evfd = 0;
    }

    io_uring_queue_exit(&m_ring);
}

bool IOUringEngine::init() {
    m_evfd = ::eventfd(0, 0);
    if (m_evfd < 0) {
        const int errnoSnap = errno;
        qFatal("Failed to create eventfd object: %s (%d)", strerror(errnoSnap), errnoSnap);
        return false;
    }

    m_socketNotifier = std::make_unique<QSocketNotifier>(m_evfd, QSocketNotifier::Read);
    connect(m_socketNotifier.get(), &QSocketNotifier::activated, this, &IOUringEngine::handleEventFd);
    m_socketNotifier->setEnabled(true);


    if (const int ret = io_uring_queue_init(ringEntryCount, &m_ring, /*flags=*/ 0); ret < 0) {
        qFatal("Failed to create io_uring queue: %s (%d)", strerror(-ret), -ret);
        ::close(m_evfd);
        m_evfd = 0;
        return false;
    }

    if (const int ret = io_uring_register_eventfd(&m_ring, m_evfd); ret < 0) {
        qFatal("Failed to register eventfd for io_uring queue: %s (%d)", strerror(-ret), -ret);
        ::close(m_evfd);
        m_evfd = 0;
        return false;
    }

    return true;
}

void IOUringEngine::handleEventFd() {
    eventfd_t event = {};
    eventfd_read(m_evfd, &event);

    struct io_uring_cqe *cqe = {};

    if (const int ret = io_uring_wait_cqe(&m_ring, &cqe); ret < 0) {
        qWarning("Failed to wait for io_uring CQE: %s (%d)", strerror(-ret), -ret);
        return;
    }

    if (cqe->res < 0) {
        qWarning("Asynchronous operation has failed: %s (%d)", strerror(-cqe->res), -cqe->res);
    } else {
        qDebug("Async operation result: %d", cqe->res);
        IOUringOperation *op = static_cast<IOUringOperation *>(io_uring_cqe_get_data(cqe));
        Q_ASSERT(op != nullptr);
        op->complete(cqe->res);
    }

    io_uring_cqe_seen(&m_ring, cqe);
}

/*
QCoroIO::OpenOperation *IOUringEngine::open(int fd, FileMode mode) {

}

QCoroIO::CloseOperation *IOUringEngine::close() {

}
*/

QCoroIO::ReadOperation IOUringEngine::read(int fd, size_t size, size_t offset) {
    return QCoroIO::ReadOperation(std::make_unique<IOUringReadOperation>(&m_ring, fd, size, offset));
}

/*
QCoroIO::WriteOperation *IOUringEngine::write(int fd, const uint8_t *data, size_t data_len) {

}
*/

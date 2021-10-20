#pragma once

#include "../ioengine_p.h"

#include <filesystem>
#include <memory>

#include <liburing.h>

class QSocketNotifier;

namespace QCoroIO::Backend {

class IOUringEngine final : public QCoroIO::IOEngine {
public:
    IOUringEngine();
    ~IOUringEngine() override;

    bool init() override;

    //OpenOperation *open(int fd, FileMode mode) override;
    //CloseOperation *close() override;

    ReadOperation read(int fd, size_t size, size_t offset) override;
    //WriteOperation *write(const uint8_t *data, size_t data_len) override;

private Q_SLOTS:
    void handleEventFd();

private:
    std::unique_ptr<QSocketNotifier> m_socketNotifier;
    struct io_uring m_ring = {};
    int m_evfd = 0;
};

} // namespace QCoroIO::Backend

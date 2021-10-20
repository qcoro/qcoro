#include "ioengine_p.h"

using namespace QCoroIO;

ReadOperation::ReadOperation(std::unique_ptr<ReadOperationImpl> impl)
    : d(std::move(impl))
{}

bool ReadOperation::await_ready() const noexcept {
    return d->awaitReady();
}

void ReadOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    d->awaitSuspend(std::move(awaitingCoroutine));
}

QByteArray ReadOperation::await_resume() {
    return d->awaitResume();
}

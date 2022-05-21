#include <QObject>
#include <QTest>

#include "qcoro/coroutine.h"
#include "qcoro/qcorotask.h"

namespace helper {

template<QCoro::Awaitable T>
struct is_awaitable {
    static constexpr bool value = true;
};

template<QCoro::Awaitable T>
static constexpr bool is_awaitable_v = is_awaitable<T>::value;

}

struct TestAwaitable {
    bool await_ready() const { return true; }
    void await_suspend(std::coroutine_handle<>);
    void await_resume() const;
};


class TestConstraints : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testAwaitableConcept() {

        static_assert(helper::is_awaitable_v<QCoro::Task<void>>,
                      "Awaitable concept doesn't accept Task<T>, although it should.");
        static_assert(helper::is_awaitable_v<TestAwaitable>,
                      "Awaitable concept doesn't accept TestAwaitable, although it should.");
    }
};

QTEST_GUILESS_MAIN(TestConstraints)

#include "testconstraints.moc"

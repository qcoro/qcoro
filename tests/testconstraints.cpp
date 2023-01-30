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

} // namespace helper

struct TestAwaitable {
    bool await_ready() const { return true; }
    void await_suspend(std::coroutine_handle<>);
    void await_resume() const;
};

struct TestAwaitableWithOperatorCoAwait {
    TestAwaitable operator co_await() {
        return TestAwaitable{};
    }
};

namespace TestNS {

struct TestAwaitableWithNonmemberOperatorCoAwait {};

#if !defined(_MSC_VER) || defined(__clang__)
// This is how it's supposed to work: the operator must be defined in the same namespace
// as the argument type and is discovered via ADL.
auto operator co_await(TestAwaitableWithNonmemberOperatorCoAwait &&) {
    return TestAwaitable{};
}
#endif

} // namespace TestNS

#if defined(_MSC_VER) && !defined(__clang__)
// Unfortunately, MSVC is only able to find the operator in global namespace :(
// This is most likely a bug in MSVC.
auto operator co_await(TestNS::TestAwaitableWithNonmemberOperatorCoAwait &&) {
    return TestAwaitable{};
}
#endif

class TestConstraints : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testAwaitableConcept() {

        static_assert(helper::is_awaitable_v<QCoro::Task<void>>,
                      "Awaitable concept doesn't accept Task<T>, although it should.");
        static_assert(helper::is_awaitable_v<TestAwaitable>,
                      "Awaitable concept doesn't accept an awaitable with await member functions.");
        static_assert(helper::is_awaitable_v<TestAwaitableWithOperatorCoAwait>,
                      "Awaitable concept doesn't accept an awaitable with member operator co_await.");
        static_assert(helper::is_awaitable_v<TestNS::TestAwaitableWithNonmemberOperatorCoAwait>,
                      "Awaitable concept doesn't accept an awaitable with non-member operator co_await.");
    }
};

QTEST_GUILESS_MAIN(TestConstraints)

#include "testconstraints.moc"

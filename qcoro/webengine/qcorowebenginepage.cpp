#include "qcorowebenginepage.h"
#include "qcorosignal.h"

using namespace QCoro::detail;

namespace {

template<typename T>
struct WebEngineAwaitable {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> awaiter) noexcept { mAwaiter = awaiter; }
    T await_resume() noexcept { return mResult; }

    void resume(const T &result) {
        mResult = result;
        mAwaiter.resume();
    }

private:
    std::coroutine_handle<> mAwaiter;
    T mResult;
};

} // namespace

QCoroWebEnginePage::QCoroWebEnginePage(QWebEnginePage *page)
    : mPage(page)
{}

QCoro::Task<QWebEngineFindTextResult> QCoroWebEnginePage::findText(const QString &subString, QWebEnginePage::FindFlags options) {
    WebEngineAwaitable<QWebEngineFindTextResult> awaitable;
    mPage->findText(subString, options, [&awaitable](const auto &result) {
        awaitable.resume(result);
    });

    co_return co_await awaitable;
}

#if QT_VERSION_MAJOR == 5

QCoro::Task<bool> QCoroWebEnginePage::load(const QUrl &url) {
    auto result = qCoro(mPage, &QWebEnginePage::loadFinished);
    mPage->load(url);
    return result;
}

QCoro::Task<bool> QCoroWebEnginePage::load(const QWebEngineHttpRequest &request) {
    auto result = qCoro(mPage, &QWebEnginePage::loadFinished);
    mPage->load(request);
    return result;
}

QCoro::Task<bool> QCoroWebEnginePage::print(QPrinter *printer) {
    WebEngineAwaitable<bool> awaitable;
    mPage->print(printer, [&awaitable](bool result) {
        awaitable.resume(result);
    });

    co_return co_await awaitable;
}

QCoro::Task<QByteArray> QCoroWebEnginePage::printToPdf(const QPageLayout &layout) {
    WebEngineAwaitable<QByteArray> awaitable;
    mPage->printToPdf(layout, [&awaitable](const QByteArray &result) {
        awaitable.resume(result);
    });

    co_return co_await awaitable;
}

QCoro::Task<> QCoroWebEnginePage::save(const QString &filePath, QWebEngineDownloadItem::SavePageFormat format) {
    mPage->save(filePath, format);

}

#else

namespace {

QCoro::Task<QWebEngineLoadingInfo> handleLoadResult(QCoro::AsyncGenerator<QWebEngineLoadingInfo> &gen) {
QCORO_FOREACH(const auto &info, changeGenerator) {
        switch (info.status()) {
        case QWebEngineLoadingInfo::LoadStartedStatus:
            continue;
        case QWebEngineLoadingInfo::LoadStoppedStatus:
        case QWebEngineLoadingInfo::LoadSucceededStatus:
        case QWebEngineLoadingInfo::LoadFailedStatus:
            co_return info;
    }

#if defined(__GNUC__)
    __builtin_unreachable();
#elif defined(_MSVC_VER)
    __assume(false);
#endif

}


} // namespace

QCoro::Task<QWebEngineLoadingInfo> QCoroWebEnginePage::load(const QUrl &url) {
    auto changeGenerator = qCoroListener(mPage, &QWebEnginePage::loadingChanged);
    mPage->load(url);
    return handleLoadResult(changeGenerator);
}

QCoro::Task<QWebEngineLoadingInfo> QCoroWebEnginePage::load(const QWebEngineHttpRequest &request) {
    auto changeGenerator = qCoroListener(mPage, &QWebEnginePage::loadingChanged);
    mPage->load(request);
    return handleLoadResult(changeGenerator);
}

#endif

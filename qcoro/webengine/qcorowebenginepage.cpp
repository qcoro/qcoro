#include "qcorowebenginepage.h"
#include "qcorosignal.h"

#include <QAction>
#include <QWebEngineProfile>

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

template<typename T>
QCoro::Task<> savePage(QWebEnginePage *page, const QString &filePath, typename T::SavePageFormat saveFormat) {
    auto downloadStart = qCoro(page->profile(), &QWebEngineProfile::downloadRequested);
    page->action(QWebEnginePage::SavePage)->trigger();
    auto *download = co_await downloadStart;
    auto downloadFinished = qCoro(download, &T::finished);
    download->setSavePageFormat(saveFormat);
    switch (saveFormat) {
        case T::UnknownSaveFormat:
        case T::SingleHtmlSaveFormat:
        case T::MimeHtmlSaveFormat:
            download->setDownloadFileName(filePath);
            break;
        case T::CompleteHtmlSaveFormat:
            download->setDownloadDirectory(filePath);
            break;
    }
    download->accept();
    co_await downloadFinished;
}

} // namespace

QCoroWebEnginePage::QCoroWebEnginePage(QWebEnginePage *page)
    : mPage(page)
{}

QCoro::Task<QWebEngineFindTextResult> QCoroWebEnginePage::findText(const QString &subString, QWebEnginePage::FindFlags options) {
    auto findFinished = qCoro(mPage.data(), &QWebEnginePage::findTextFinished);
    mPage->findText(subString, options);
    co_return co_await findFinished;
}

QCoro::Task<QVariant> QCoroWebEnginePage::runJavaScript(const QString &scriptSource) {
    WebEngineAwaitable<QVariant> awaitable;
    mPage->runJavaScript(scriptSource, [&awaitable](const QVariant &result) {
        awaitable.resume(result);
    });
    co_return co_await awaitable;
}

QCoro::Task<QVariant> QCoroWebEnginePage::runJavaScript(const QString &scriptSource, quint32 worldId) {
    WebEngineAwaitable<QVariant> awaitable;
    mPage->runJavaScript(scriptSource, worldId, [&awaitable](const QVariant &result) {
        awaitable.resume(result);
    });
    co_return co_await awaitable;
}

QCoro::Task<QString> QCoroWebEnginePage::toHtml() const {
    WebEngineAwaitable<QString> awaitable;
    mPage->toHtml([&awaitable](const QString &result) {
        awaitable.resume(result);
    });
    co_return co_await awaitable;
}

QCoro::Task<QString> QCoroWebEnginePage::toPlainText() const {
    WebEngineAwaitable<QString> awaitable;
    mPage->toPlainText([&awaitable](const QString &result) {
        awaitable.resume(result);
    });
    co_return co_await awaitable;
}

#if QT_VERSION_MAJOR == 5

QCoro::Task<bool> QCoroWebEnginePage::load(const QUrl &url) {
    auto result = qCoro(mPage.data(), &QWebEnginePage::loadFinished);
    mPage->load(url);
    return result;
}

QCoro::Task<bool> QCoroWebEnginePage::load(const QWebEngineHttpRequest &request) {
    auto result = qCoro(mPage.data(), &QWebEnginePage::loadFinished);
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
    mPage->printToPdf([&awaitable](const QByteArray &result) {
        awaitable.resume(result);
    }, layout);

    co_return co_await awaitable;
}

QCoro::Task<> QCoroWebEnginePage::save(const QString &filePath, QWebEngineDownloadItem::SavePageFormat format) const {
    return savePage<QWebEngineDownloadItem>(mPage.data(), filePath, format);
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

QCoro::Task<QByteArray> QCoroWebEnginePage::printToPdf(const QPageLayout &layout, const QPageRanges &ranges) {
    WebEngineAwaitable<QByteArray> awaitable;
    mPage->printToPdf([&awaitable](const auto &result) {
        awaitable.resume(result);
    }, layout, ranges);
    co_return co_await awaitable;
}

QCoro::Task<> QCoroWebEnginePage::save(const QString &filePath, QWebEngineDownloadRequest::SavePageFormat saveFormat) const {
    return savePage<QWebEngineDownloadRequest>(mPage.get(), filePath, saveFormat);
}

#endif


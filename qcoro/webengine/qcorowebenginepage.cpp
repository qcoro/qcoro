#include "qcorowebenginepage.h"
#include "qcorosignal.h"

#include <QAction>
#include <QWebEngineProfile>

#include <optional>

using namespace QCoro::detail;

namespace {

template<typename T>
struct WebEngineAwaitable {
    explicit WebEngineAwaitable() = default;
    Q_DISABLE_COPY_MOVE(WebEngineAwaitable)

    bool await_ready() const noexcept {
        return mResult.has_value();
    }
    void await_suspend(std::coroutine_handle<> awaiter) noexcept {
        mAwaiter = awaiter;
    }
    T await_resume() noexcept {
        return std::move(*mResult);
    }

    void resume(const T &result) {
        mResult = result;
        if (mAwaiter) {
            mAwaiter.resume();
        }
    }

private:
    std::coroutine_handle<> mAwaiter;
    std::optional<T> mResult;
};

template<typename T, typename FinishedSignal>
QCoro::Task<> savePage(QWebEnginePage *page, const QString &filePath, typename T::SavePageFormat saveFormat, FinishedSignal finishedSignal) {
    auto downloadStart = qCoro(page->profile(), &QWebEngineProfile::downloadRequested);
    page->action(QWebEnginePage::SavePage)->trigger();
    auto *download = co_await downloadStart;
    auto downloadFinished = qCoro(download, finishedSignal);
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
#if QT_VERSION_MAJOR == 6
    WebEngineAwaitable<QWebEngineFindTextResult> awaitable;
    mPage->findText(subString, options, [&awaitable](const auto &result) {
        awaitable.resume(result);
    });
    co_return co_await awaitable;
#else
    auto findFinished = qCoro(mPage.data(), &QWebEnginePage::findTextFinished);
    mPage->findText(subString, options);
    co_return co_await findFinished;
#endif
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
    return savePage<QWebEngineDownloadItem>(mPage.data(), filePath, format, &QWebEngineDownloadItem::finished);
}

#else // QT_VERSION_MAJOR

namespace {

QCoro::Task<QWebEngineLoadingInfo> handleLoadResult(QCoro::AsyncGenerator<QWebEngineLoadingInfo> gen) {
    qDebug() << "HANDLE RESULT";
    QCORO_FOREACH(const auto &info, gen) {
        qDebug() << "LOADING" << info.status();
        switch (info.status()) {
        case QWebEngineLoadingInfo::LoadStartedStatus:
            continue;
        case QWebEngineLoadingInfo::LoadStoppedStatus:
        case QWebEngineLoadingInfo::LoadSucceededStatus:
        case QWebEngineLoadingInfo::LoadFailedStatus:
            co_return info;
        }
    }

    Q_UNREACHABLE();
}

} // namespace

QCoro::Task<QWebEngineLoadingInfo> QCoroWebEnginePage::load(const QUrl &url) {
    auto changeGenerator = qCoroSignalListener(mPage.get(), &QWebEnginePage::loadingChanged);
    mPage->load(url);
    return handleLoadResult(std::move(changeGenerator));
}

QCoro::Task<QWebEngineLoadingInfo> QCoroWebEnginePage::load(const QWebEngineHttpRequest &request) {
    auto changeGenerator = qCoroSignalListener(mPage.get(), &QWebEnginePage::loadingChanged);
    mPage->load(request);
    return handleLoadResult(std::move(changeGenerator));
}

QCoro::Task<QByteArray> QCoroWebEnginePage::printToPdf(const QPageLayout &layout, const QPageRanges &ranges) {
    WebEngineAwaitable<QByteArray> awaitable;
    mPage->printToPdf([&awaitable](const auto &result) {
        awaitable.resume(result);
    }, layout, ranges);
    co_return co_await awaitable;
}

QCoro::Task<> QCoroWebEnginePage::save(const QString &filePath, QWebEngineDownloadRequest::SavePageFormat saveFormat) const {
    return savePage<QWebEngineDownloadRequest>(mPage.get(), filePath, saveFormat, &QWebEngineDownloadRequest::isFinishedChanged);
}

#endif // QT_VERSION_MAJOR


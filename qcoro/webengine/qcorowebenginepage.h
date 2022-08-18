#pragma once

#include "qcorotask.h"

#include <QPointer>
#include <QWebEnginePage>
#if QT_VERSION_MAJOR == 5
#include <QWebEngineDownloadItem>
#else
#include <QWebEngineDownloadRequest>
#include <QWebEngineLoadingInfo>
#endif
#include <QWebEngineFindTextResult>

class QWebEnginePage;

namespace QCoro::detail {

class QCoroWebEnginePage {
public:
    explicit QCoroWebEnginePage(QWebEnginePage *page);

    QCoro::Task<QWebEngineFindTextResult> findText(const QString &subString, QWebEnginePage::FindFlags options = {});

#if QT_VERSION_MAJOR == 5
    QCoro::Task<bool> load(const QUrl &url);
    QCoro::Task<bool> load(const QWebEngineHttpRequest &request);
#else
    QCoro::Task<QWebEngineLoadingInfo> load(const QUrl &url);
    QCoro::Task<QWebEngineLoadingInfo> load(const QWebEngineHttpRequest &request);
#endif

#if QT_VERSION_MAJOR == 5
    QCoro::Task<bool> print(QPrinter *printer);

    QCoro::Task<QByteArray> printToPdf(const QPageLayout &layout = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF()));
#else
    QCoro::Task<QByteArray> printToPdf(const QPageLayout &layout = QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF()),
                                       const QPageRanges &ranges = {});
#endif

    QCoro::Task<QVariant> runJavaScript(const QString &scriptSource);
    QCoro::Task<QVariant> runJavaScript(const QString &scriptSource, quint32 worldId = 0);

#if QT_VERSION_MAJOR == 5
    QCoro::Task<> save(const QString &filePath,
                       QWebEngineDownloadItem::SavePageFormat format = QWebEngineDownloadItem::MimeHtmlSaveFormat) const;
#else
    QCoro::Task<> save(const QString &filePath,
                       QWebEngineDownloadRequest::SavePageFormat format = QWebEngineDownloadRequest::MimeHtmlSaveFormat) const;
#endif

    QCoro::Task<QString> toPlainText() const;
    QCoro::Task<QString> toHtml() const;

private:
    QPointer<QWebEnginePage> mPage;
};

} // namespace QCoro

inline auto qCoro(QWebEnginePage *page) {
    return QCoro::detail::QCoroWebEnginePage(page);
}

inline auto qCoro(QWebEnginePage &page) {
    return QCoro::detail::QCoroWebEnginePage(&page);
}

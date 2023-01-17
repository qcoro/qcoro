// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoro/network/qcoronetworkreply.h"
#include "qcoro/qcorotask.h"

#include <QApplication>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion" // QSizePolicy seems problematic...
#endif // __clang__
#include <QBoxLayout>
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__
#include <QMainWindow>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>

#include <memory>

// Programming langauges
static const QUrl wikiUrl =
    QUrl{QStringLiteral("https://www.wikidata.org/wiki/Special:EntityData/Q9143.json")};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() {
        mPb = new QProgressBar();
        mPb->setVisible(false);
        mPb->setMinimumWidth(200);
        mPb->setMinimum(0);
        mPb->setMaximum(0);

        mBtn = new QPushButton(tr("Start Download"));
        connect(mBtn, &QPushButton::clicked, this, &MainWindow::start);

        auto *vbox = new QVBoxLayout();
        vbox->addStretch(1);
        vbox->addWidget(mPb);
        vbox->addWidget(mBtn);
        vbox->addStretch(1);

        auto *hbox = new QHBoxLayout();
        hbox->addStretch(1);
        hbox->addLayout(vbox);
        hbox->addStretch(1);

        QWidget *w = new QWidget;
        w->setLayout(hbox);
        setCentralWidget(w);
    }

private Q_SLOTS:
    QCoro::Task<> start() {
        mPb->setVisible(true);
        mBtn->setEnabled(false);
        mBtn->setText(tr("Downloading ..."));

        std::unique_ptr<QNetworkReply> reply(co_await mNam.get(QNetworkRequest{wikiUrl}));
        if (reply->error()) {
            QMessageBox::warning(
                this, tr("Network request error"),
                tr("Error occured during network request. Error code: %1").arg(reply->error()));
            co_return;
        }

        mPb->setVisible(false);
        mBtn->setEnabled(true);
        mBtn->setText(tr("Done, download again"));
    }

private:
    QNetworkAccessManager mNam;
    QPushButton *mBtn = {};
    QProgressBar *mPb = {};
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.showNormal();
    return app.exec();
}

#include "main.moc"

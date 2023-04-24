// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: MIT

#include "qcoroqml.h"
#include "qcorotask.h"
#include "qcorotimer.h"
#include "qcoroqmltask.h"
#include "qcorofuture.h"

#include <QTest>
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <chrono>

using namespace std::chrono_literals;

class QmlObject : public QObject {
    Q_OBJECT

public:
    Q_INVOKABLE QCoro::QmlTask startTimer() {
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->start(1s);
        return [timer]() -> QCoro::Task<> {
            co_await timer;
        }();
    }

    Q_INVOKABLE QCoro::QmlTask qmlTaskFromTimer() {
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->start(1s);
        return timer;
    }

    Q_INVOKABLE QCoro::QmlTask qmlTaskFromFuture() {
        QFutureInterface<QString> interface;
        interface.reportResult(QStringLiteral("Success"));
        interface.reportFinished();
        return interface.future();
    }

    Q_INVOKABLE void reportTestSuccess() {
        numTestsPassed++;

        if (numTestsPassed == 4) { // Number of java script functions that call reportTestSuccess
            Q_EMIT success();
        }
    }

    Q_SIGNAL void success();

private:
    int numTestsPassed = 0;
};

class QCoroQmlTaskTest : public QObject {
    Q_OBJECT

private:
    Q_SLOT void testQmlCallback() {
        QQmlApplicationEngine engine;
        qmlRegisterSingletonType<QmlObject>("qcoro.test", 0, 1, "QmlObject", [](QQmlEngine *, QJSEngine *) {
            return new QmlObject();
        });

        QCoro::Qml::registerTypes();

        engine.loadData(R"(
import qcoro.test 0.1
import QCoro 0
import QtQuick 2.7

QtObject {
    property string value: QmlObject.qmlTaskFromFuture().await("Loading...").value

    property string valueWithoutIntermediate: QmlObject.qmlTaskFromFuture().await().value

    onValueChanged: {
        if (value == "Success") {
            console.log("awaiting finished")
            QmlObject.reportTestSuccess()
        }
    }

    Component.onCompleted: {
        QmlObject.startTimer().then(() => {
            console.log("QCoro::Task JavaScript callback called")
            QmlObject.reportTestSuccess()
        })

        QmlObject.qmlTaskFromTimer().then(() => {
            console.log("QTimer JavaScript callback called")
            QmlObject.reportTestSuccess()
        })

        QmlObject.qmlTaskFromFuture().then(() => {
            console.log("QFuture JavaScript callback called")
            QmlObject.reportTestSuccess()
        })
    }
}
)");
        auto *object = engine.singletonInstance<QmlObject *>(qmlTypeId("qcoro.test", 0, 1, "QmlObject"));

        auto *timeout = new QTimer(this);
        timeout->setSingleShot(true);
        timeout->setInterval(2s);
        timeout->start();

        bool running = true;
        // End the event loop normally
        connect(object, &QmlObject::success, this, [&]() {
            timeout->stop();
            running = false;
        });

        // Crash the test in case the timeout was reachaed without the callback being called
        connect(timeout, &QTimer::timeout, this, [&]() {
#if defined(Q_CC_CLANG) && defined(Q_OS_WINDOWS)
            running = false;
            QEXPECT_FAIL("", "QTBUG-91768", Abort);
            QVERIFY(false);
            return;
#else
            QFAIL("Timeout waiting for QML continuation to be called");
#endif
        });
        while (running) {
            QCoreApplication::processEvents();
        }
    }
};

QTEST_GUILESS_MAIN(QCoroQmlTaskTest)

#include "qcoroqmltask.moc"

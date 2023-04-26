<!--
SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# Test Module

The `Test` module contains coroutine-friendly versions of tests macros
from the [QtTest][qdoc-qtest] module.

## CMake Usage

```cmake
find_package(QCoro6 COMPONENTS Test)
...
target_link_libraries(my-target QCoro::Test)
```

## QMake Usage

```
QT += QCoroTest
```

## Test Macros

The module contains a bunch of test macros that behave identically to their QtTest
counterparts, the only difference being that they can be used inside a coroutine.

```cpp
#include <QCoroTest>
```

* [`QCORO_COMPARE(actual, expected)`][qdoc-qcompare]
* [`QCORO_EXPECT_FAIL(dataIndex, comment, mode)`][qdoc-qexpect-fail]
* [`QCORO_FAIL(message)`][qdoc-qfail]
* [`QCORO_SKIP(description)`][qdoc-qskip]
* [`QCORO_TEST(data, testElement)`][qdoc-qtest]
* [`QCORO_TRY_COMPARE(actual, expected)`][qdoc-qtry-compare]
* [`QCORO_TRY_COMPARE_WITH_TIMEOUT(actual, expected, timeout)`][qdoc-qtry-compare-with-timeout]
* [`QCORO_TRY_VERIFY2(condition, message)`][qdoc-qtry-verify2]
* [`QCORO_TRY_VERIFY(condition)`][qdoc-qtry-verify]
* [`QCORO_TRY_VERIFY2_WITH_TIMEOUT(condition, message, timeout)`][qdoc-qtry-verify2-with-timeout]
* [`QCORO_TRY_VERIFY_WITH_TIMEOUT(condition, timeout)`][qdoc-qtry-verify-with-timeout]
* [`QCORO_VERIFY2(condition, message)`][qdoc-qverify2]
* [`QCORO_VERIFY(condition)`][qdoc-qverify]
* [`QCORO_VERIFY_EXCEPTION_THROWN(expression, exceptionType)`][qdoc-qverify-exception-thrown]


[qdoc-qtest]: https://doc.qt.io/qt-5/qttest-index.html
[qdoc-qcompare]: https://doc.qt.io/qt-5/qtest.html#QCOMPARE
[qdoc-qexpect-fail]: https://doc.qt.io/qt-5/qtest.html#QEXPECT_FAIL
[qdoc-qfail]: https://doc.qt.io/qt-5/qtest.html#QFAIL
[qdoc-qskip]: https://doc.qt.io/qt-5/qtest.html#QSKIP
[qdoc-qtest]: https://doc.qt.io/qt-5/qtest.html#QTEST
[qdoc-qtry-compare]: https://doc.qt.io/qt-5/qtest.html#QTRY_COMPARE
[qdoc-qtry-compare-with-timeout]: https://doc.qt.io/qt-5/qtest.html#QTRY_COMPARE_WITH_TIMEOUT
[qdoc-qtry-verify2]: https://doc.qt.io/qt-5/qtest.html#QTRY_VERIFY2
[qdoc-qtry-verify]: https://doc.qt.io/qt-5/qtest.html#QTRY_VERIFY
[qdoc-qtry-verify2-with-timeout]: https://doc.qt.io/qt-5/qtest.html#QTRY_VERIFY2_WITH_TIMEOUT
[qdoc-qtry-verify-with-timeout]: https://doc.qt.io/qt-5/qtest.html#QTRY_VERIFY_WITH_TIMEOUT
[qdoc-qverify2]: https://doc.qt.io/qt-5/qtest.html#QVERIFY2
[qdoc-qverify]: https://doc.qt.io/qt-5/qtest.html#QVERIFY
[qdoc-qverify-exception-thrown]: https://doc.qt.io/qt-5/qtest.html#QVERIFY_EXCEPTION_THROWN



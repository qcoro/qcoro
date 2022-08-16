<!--
SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# ImageProvider

{{ doctable("Quick", "QCoroImageProvider") }}

Coroutines based implementation of [`QQuickImaqeProvider`][qdoc-imageprovider].

To use `QCoro::ImageProvider`, you need to create a subclass of it, and implement the `asyncRequestImage` function, as shown in the example below:
```cpp
#include <QCoro/QCoroImageProvider>

class IconImageProvider : public QCoro::ImageProvider
{
public:
    QCoro::Task<QImage> asyncRequestImage(const QString &id, const QSize &) override;
};
```

The subclass [can be registered with a `QQmlEngine`][qdoc-addimageprovider] like any `QQuickImageProvider` subclass.

[qdoc-addimageprovider]: https://doc.qt.io/qt-5/qqmlengine.html#addImageProvider
[qdoc-imageprovider]: https://doc.qt.io/qt-5/qquickimageprovider.html

#include "qcoroasyncsqldriverplugin_p.h"
#include "psql_driver.h"

class QCoroAsyncPsqlPlugin : public QCoro::AsyncSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "cz.dvratil.qcoro.AsyncSqlDriverPlugin" FILE "psql.json")
public:
    explicit QCoroAsyncPsqlPlugin(QObject *parent = nullptr)
        : QCoro::AsyncSqlDriverPlugin(parent) {}

    QCoro::AsyncSqlDriver *create(const QString &key) override
    {
        if (key == u"QPSQL") {
            return new QCoroAsyncPsqlDriver();
        }
        return nullptr;
    }
};

#include "main.moc"
#include "psql_types.h"

using namespace QCoroPsql;

QMetaType decodePsqlType(int oid)
{
    switch (oid) {
    case BoolOID:
        return QMetaType(QMetaType::Bool);
    case Int8OID:
        return QMetaType(QMetaType::LongLong);
    case Int2OID:
    case Int4OID:
    case OIDOID:
    case RegProcedureOID:
    case XIDOID:
    case CIDOID:
        return QMetaType(QMetaType::Int);
    case NumericOID:
    case Float4OID:
    case Float8OID:
        return QMetaType(QMetaType::Double);
    case AbsoluteTimeOID:
    case RelativeTimeOID:
    case DateOID:
        return QMetaType(QMetaType::QDate);
    case TimeOID:
    case TimeTZOID:
        return QMetaType(QMetaType::QTime);
    case TimestampOID:
    case TimestampTZOID:
        return QMetaType(QMetaType::QDateTime);
    case ByteAOID:
        return QMetaType(QMetaType::QByteArray);
    default:
        return QMetaType(QMetaType::QString);
    }
}


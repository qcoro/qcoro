#pragma once

#include <QMetaType>

#include <libpq-fe.h>

#include <memory>

Q_DECLARE_OPAQUE_POINTER(PGresult *)

namespace QCoroPsql {

static constexpr int BoolOID = 16;
static constexpr int Int8OID = 20;
static constexpr int Int2OID = 21;
static constexpr int Int4OID = 23;
static constexpr int OIDOID = 26;
static constexpr int RegProcedureOID = 24;
static constexpr int XIDOID = 28;
static constexpr int CIDOID = 29;
static constexpr int NumericOID = 1700;
static constexpr int Float4OID = 700;
static constexpr int Float8OID = 701;
static constexpr int AbsoluteTimeOID = 702;
static constexpr int RelativeTimeOID = 703;
static constexpr int DateOID = 1082;
static constexpr int TimeOID = 1083;
static constexpr int TimeTZOID = 1266;
static constexpr int TimestampOID = 1114;
static constexpr int TimestampTZOID = 1184;
static constexpr int ByteAOID = 17;
static constexpr int BitOID = 1560;
static constexpr int VarBitOID = 1562;


static constexpr size_t VARHDRSZ = sizeof(uint32_t);

QMetaType decodePsqlType(int oid);

using PGresultPtr = std::unique_ptr<PGresult, decltype(&PQclear)>;

inline PGresultPtr makePGResultPtr(PGresult *result)
{
    return PGresultPtr(result, PQclear);
}

using StatementId = ssize_t;
constexpr StatementId InvalidStatementId = 0;

} // namespace QCoroPsql
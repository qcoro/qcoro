#pragma once

#include <QFlags>

namespace QCoroIO {

Q_NAMESPACE


enum class FileMode {
    ReadOnly = 0x01,
    WriteOnly = 0x02,
    ReadWrite = ReadOnly | WriteOnly,
    Append = 0x04,
    Truncate = 0x08,
    NewOnly = 0x10,
    ExistingOnly = 0x20
};

Q_DECLARE_FLAGS(FileModes, FileMode);
Q_FLAG_NS(FileModes)

} // namespace QCoroIO

Q_DECLARE_OPERATORS_FOR_FLAGS(QCoroIO::FileModes)

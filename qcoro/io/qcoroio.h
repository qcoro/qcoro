#pragma once

namespace QCoroIO {

enum class FileMode {
    ReadOnly = 0x01,
    WriteOnly = 0x02,
    ReadWrite = ReadOnly | WriteOnly,
    Append = 0x04,
    Truncate = 0x08
};

} // namespace QCoroIO

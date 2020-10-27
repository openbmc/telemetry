#pragma once

#include "interfaces/json_storage.hpp"

#include <gmock/gmock.h>

namespace interfaces
{

inline void PrintTo(const JsonStorage::FilePath& o, std::ostream* os)
{
    (*os) << static_cast<std::filesystem::path>(o);
}
inline void PrintTo(const JsonStorage::DirectoryPath& o, std::ostream* os)
{
    (*os) << static_cast<std::filesystem::path>(o);
}

} // namespace interfaces
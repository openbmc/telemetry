#pragma once

#include "types/duration_types.hpp"

#include <gmock/gmock.h>

template <class Ratio>
inline void PrintTo(const std::chrono::duration<uint64_t, Ratio>& o,
                    std::ostream* os)
{
    (*os) << std::chrono::duration_cast<Milliseconds>(o).count() << "us";
}

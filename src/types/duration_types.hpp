#pragma once

#include <chrono>

using Seconds = std::chrono::duration<uint64_t>;
using Milliseconds = std::chrono::duration<uint64_t, std::milli>;
using Microseconds = std::chrono::duration<uint64_t, std::micro>;

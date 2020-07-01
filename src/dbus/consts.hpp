#pragma once

#include <chrono>
#include <cstdint>

namespace app
{

constexpr unsigned maxReports = 30u;
constexpr std::chrono::milliseconds pollRateResolution = 100ms;
} // namespace app

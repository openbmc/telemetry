#pragma once

#include <chrono>
#include <cstdint>

namespace app
{

constexpr uint32_t maxReports = 30u;
constexpr std::chrono::milliseconds pollRateResolution = 100ms;
} // namespace app

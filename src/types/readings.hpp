#pragma once

#include "utils/labeled_tuple.hpp"
#include "utils/tstring.hpp"

using ReadingData = std::tuple<std::string, double, uint64_t>;
using Readings = std::tuple<uint64_t, std::vector<ReadingData>>;

using LabeledReadingData =
    utils::LabeledTuple<ReadingData, utils::tstring::MetricProperty,
                        utils::tstring::MetricValue, utils::tstring::Timestamp>;

using LabeledReadings =
    utils::LabeledTuple<std::tuple<uint64_t, std::vector<LabeledReadingData>>,
                        utils::tstring::Timestamp, utils::tstring::Readings>;

namespace utils
{

LabeledReadings toLabeledReadings(const Readings&);
Readings toReadings(const LabeledReadings&);

} // namespace utils

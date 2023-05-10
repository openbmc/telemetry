#include "types/readings.hpp"

#include "utils/transform.hpp"

namespace utils
{

namespace ts = utils::tstring;

LabeledReadings toLabeledReadings(const Readings& readings)
{
    return LabeledReadings{std::get<0>(readings),
                           utils::transform(std::get<1>(readings),
                                            [](const auto& readingData) {
        return LabeledReadingData{readingData};
                           })};
}

Readings toReadings(const LabeledReadings& labeledReadings)
{
    return Readings{labeledReadings.at_label<ts::Timestamp>(),
                    utils::transform(labeledReadings.at_label<ts::Readings>(),
                                     [](const auto& labeledReadingData) {
        return labeledReadingData.to_tuple();
                    })};
}

} // namespace utils

#include "report_types.hpp"

#include "utils/transform.hpp"

ReadingParameters
    toReadingParameters(const std::vector<LabeledMetricParameters>& labeled)
{
    namespace ts = utils::tstring;

    return utils::transform(
        labeled, [](const LabeledMetricParameters& metricParams) {
            return ReadingParameters::value_type(
                utils::transform(
                    metricParams.at_label<ts::SensorPath>(),
                    [](const LabeledSensorInfo& sensorParameters) {
                        return std::tuple<sdbusplus::message::object_path,
                                          std::string>(
                            sensorParameters.at_label<ts::Path>(),
                            sensorParameters.at_label<ts::Metadata>());
                    }),
                utils::enumToString(metricParams.at_label<ts::OperationType>()),
                utils::enumToString(
                    metricParams.at_label<ts::CollectionTimeScope>()),
                metricParams.at_label<ts::CollectionDuration>().t.count());
        });
}

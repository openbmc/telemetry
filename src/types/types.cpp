#include "types.hpp"

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
                    [](const LabeledSensorParameters& sensorParameters) {
                        return sdbusplus::message::object_path(
                            sensorParameters.at_label<ts::Path>());
                    }),
                utils::enumToString(metricParams.at_label<ts::OperationType>()),
                metricParams.at_label<ts::Id>(),
                metricParams.at_label<ts::MetricMetadata>(),
                utils::enumToString(
                    metricParams.at_label<ts::CollectionTimeScope>()),
                metricParams.at_label<ts::CollectionDuration>().t.count());
        });
}

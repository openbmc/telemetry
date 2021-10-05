#include "add_report_params.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

void printMetricParams(const generated::MetricParams& params)
{
    std::cout << "id: " << params.id << std::endl;
    std::cout << "collectionDuration: " << params.collectionDuration
              << std::endl;
    std::cout << "collectionTimescope: "
              << generated::collectionTimescopeToString(
                     params.collectionTimescope)
              << std::endl;
    std::cout << "metadata: " << params.metadata << std::endl;
    std::cout << "operationType: "
              << generated::operationTypeToString(params.operationType)
              << std::endl;
    std::cout << "sensorPaths.size(): " << params.sensorPaths.size()
              << std::endl;
    for (size_t idx = 0; idx < params.sensorPaths.size(); idx++)
    {
        std::cout << "sensorPaths[" << idx << "]: " << params.sensorPaths[idx]
                  << std::endl;
    }
}

int main()
{
    std::ifstream ifs("data/data1.json");
    nlohmann::json json_data = nlohmann::json::parse(ifs);
    auto reportParams = json_data.get<generated::AddReportParams>();

    std::cout << "First Metric Paramater - All values filled:\n";
    printMetricParams(reportParams.metricParameters[0]);

    std::cout << "\nSecond Metric Paramater - All values defaulted:\n";
    printMetricParams(reportParams.metricParameters[1]);

    return 0;
}

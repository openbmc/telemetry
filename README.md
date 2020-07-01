# MonitoringService
This component implements middleware for sensors and metrics aggregation.

## Capabilities
This application is implementation of MonitoringService proposed in design`[1]`.

It's responsible for:
- on-demand creation of metric reports,
  - aggregated sets of sensor values available in system `[2]`,
- access to metric report in both  pull and push model (triggers),
- run-time monitoring of sensor`[3]` updates.

## Use-cases
- generic and centralized way to observe telemetry data inside system
- back-end for Redfish TelemetryService`[4]`

## TelemetryService quick-start
This chapter covers basic information about Redfish TelemetryService. Please see linked documents for more detailed information.

### Definitions
* `TelemetryService`
  * Root node of configuration. Contains links to all TelemetryService subnodes and system-wide properties and configuration.
* `Metric`
  * Concrete measurable value, eg. temperature, power, voltage etc.
* `MetricDefinition`
  * Metric metadata - unit, precision, context etc.
* `MetricReport`
  * Snapshot of Metric values grouped together in one resource.
* `MetricReportDefinition`
  * Configuration of MetricReport generation. It specifies what metrics should be monitored, how often it should be updated, what actions should be performed on MetricReport update etc.

### Basic properties
This is list of the most important properties for basic TelemetryService utilization. It's limited to cover core functionalities and use cases. More detailed descriptions can be found in the documentation `[5]`.

#### TelemetryService
`/redfish/v1/TelemetryService`
* `Status` - shows if TelemetryService is available and ready to operate
* `SupportedCollectionFunctions` - defines what operations could be performed on Metrics

#### MetricDefinition
`/redfish/v1/TelemetryService/MetricDefinitions/{MetricDefinitionId}`
* `MetricProperties` - list of URIs (Metrics) that this MetricDefinition represents
* `MetricType` - specifies nature of measurement (boolean, numeric, counter etc.)
* `Units` - defines metric units (Watts/Volts etc.)
* `IsLinear` - indication if metric is linear vs. non-linear
* `DiscreteValues` - array containing all possible values for discrete metrics

#### MetricReport
`/redfish/v1/TelemetryService/MetricReports/{MetricReportId}`
* `MetricReportDefinition` - link to the MetricReportDefinition that created this concrete report
* `MetricValues` - array of metric values
  * `MetricDefinition` - link to MetricDefinition describing given value in detail
  * `MetricId` - custom identifier
  * `MetricProperty` - URI from which the metric value was retrieved
  * `MetricValue` - value
  * `Timestamp` - time at which this metric value was retrieved
* `Timestamp` - time at which this MetricReport was created

#### MetricReportDefinition
`/redfish/v1/TelemetryService/MetricReportDefinitions/{MetricReportDefinitionId}`
* `MetricReportDefinitionType` - configuration of trigger that updates the resultant MetricReport (on metrics change, periodically, on request)
* `Metrics` - array of metrics to monitor with additional settings (like id, calculation algorithm etc)
* `MetricReport` - link to MetricReport - concrete snapshot of the metric values
* `ReportActions` - actions to perform when MetricReport is created (send event, log in collection etc.)
* `ReportUpdates` - definition of how the report should behave when new metric data is available (overwrite values, generate new report etc.)

### Typical use-case
Following instruction covers creation of basic MetricReport which is updated periodically and available to read over Redfish with GET request.

1. User lists all `MetricDefinition` in `/redfish/v1/TelemetryService/MetricDefinitions` collection.
   1. URIs to metrics of interest are taken from `MetricProperties`
2. User creates `MetricReportDefinition` for chosen group of metrics
   1. Performs `POST` operation on `/redfish/v1/TelemetryService/MetricReportDefinitions` populated with MetricReportDefinition of choice, eg:
    ```json
    {
        "Id": "SimplePeriodicReport",
        "Metrics": [
            {
                "MetricId": "BMC Temp",
                "MetricProperties": ["/redfish/v1/Chassis/Board/Thermal#/Temperatures/0/ReadingCelsius"]
            },
            {
                "MetricId": "P1V8_PCH",
                "MetricProperties": ["/redfish/v1/Chassis/Board/Power#/Voltages/3/ReadingVolts"]
            }],
        "MetricReportDefinitionType": "Periodic",
        "ReportActions": ["RedfishEvent", "LogToMetricReportsCollection"],
        "Schedule": {
            "RecurrenceInterval": "PT0.1S"
    }
    ```
3. He inspects resultant MetricReportDefinition, in this case it will be at: `/redfish/v1/TelemetryService/MetricReportDefinitions/SimplePeriodicReport`
   1. Verifies the contents and URI of MetricReport.
4. By default all MetricReportDefinitions create respective report at `/redfish/v1/TelemetryService/MetricReports/{MetricReportDefinitionId}`, in this case it would be `/redfish/v1/TelemetryService/MetricReports/SimplePeriodicReport`

## References
1. [OpenBMC platform telemetry design](https://github.com/openbmc/docs/blob/master/designs/telemetry.md)
2. [Sensor support for OpenBMC](https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md)
3. [dbus-sensors](https://github.com/openbmc/dbus-sensors)
4. [Redfish TelemetryService](https://redfish.dmtf.org/schemas/v1/TelemetryService.json)
5. [Redfish Schema Guide](https://www.dmtf.org/sites/default/files/standards/documents/DSP2046_2019.3.pdf)

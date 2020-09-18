# Telemetry #
This component implements middleware for sensors and metrics aggregation.


## Capabilities ##
This application is implementation of Telemetry proposed in design`[1]`.

It's responsible for:
- on-demand creation of metric reports,
  - aggregated sets of sensor values available in system `[2]`,
- access to metric report in both  pull and push model (triggers),
- run-time monitoring of sensor`[3]` updates.

## Use-cases ##
- generic and centralized way to observe telemetry data inside system
- back-end for Redfish TelemetryService`[4]`

## References ##
1. [OpenBMC platform telemetry design](https://github.com/openbmc/docs/blob/master/designs/telemetry.md)
2. [Sensor support for OpenBMC](https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md)
3. [dbus-sensors](https://github.com/openbmc/dbus-sensors)
4. [Redfish TelemetryService](https://redfish.dmtf.org/schemas/v1/TelemetryService.json)

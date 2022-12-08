# Telemetry

This component implements middleware for sensors and metrics aggregation.

## Capabilities

This application is implementation of Telemetry proposed in OpenBMC design docs
`[1]`.

It's responsible for:

- on-demand creation of metric reports,
  - aggregated sets of sensor values available in system `[2]`,
- access to metric report in both pull and push model (triggers),
- run-time monitoring of sensor`[3]` updates.

## Use-cases

- generic and centralized way to observe telemetry data inside system
- back-end for Redfish TelemetryService`[4]`

## How to build

There are two way to build telemetry service:

- using bitbake in yocto environment
- using meson as native build

To build it using bitbake follow the guide from OpenBMC docs`[5]`. To build it
using meson follow the quick guide to install meson`[6]` and then run below
commands

```
meson build
cd build
ninja
```

After successful build you should be able to run telemetry binary or start unit
tests

```
./tests/telemetry-ut
./telemetry
```

In case if system is missing boost dependency, it is possible to build it
locally and set BOOST_ROOT environment variable to location of built files for
meson. After this change meson should be able to detect boost dependency. See
`[7]` for more details.

## Notes

More information can be found in OpenBMC docs repository `[8]`.

## References

1. [OpenBMC platform telemetry design](https://github.com/openbmc/docs/blob/master/designs/telemetry.md)
2. [Sensor support for OpenBMC](https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md)
3. [dbus-sensors](https://github.com/openbmc/dbus-sensors)
4. [Redfish TelemetryService](https://redfish.dmtf.org/schemas/v1/TelemetryService.json)
5. [Yocto-development](https://github.com/openbmc/docs/blob/master/yocto-development.md)
6. [Meson-Quick-Guide](https://mesonbuild.com/Quick-guide.html)
7. [Meson-Boost-dependency](https://mesonbuild.com/Dependencies.html#boost)
8. [OpenBMC-docs-repository](https://github.com/openbmc/docs)

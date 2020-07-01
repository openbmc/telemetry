# Code overview
This document provides general understanding of telemetry code structure and
internal workflows.

It also provides basic understanding undersanding of classes responsibilities
and expectations. In particular - it lists all features which are still missing
from original design.

Please refer to README.md for links to high level documentation. This
application is adjusted to comply mostly with
[OpenBMC Telemetry Design](https://github.com/openbmc/docs/blob/master/designs/telemetry.md)
, which specifies APIs, types, supported modes etc.

## File structure
###  \
*   `.clang-format` - configuration file to clang-format tool, compatible with
        latest OpenBMC standards
*   `xyz.openbmc_project.MonitoringService.service` - systemd service file,
        responsible for configuring system to start application at boot time
*   `.gitignore` - contains paths and files which should not be commited to
        repository (intermediate files, binaries, generated headers etc.)
*   `CMakeLists.txt` - main Cmake file
*   `README.md` `OVERVIEW.md` `LICENSE` `MAINTAINERS` - project documentation


### \cmake
Contains files used by CMake build system. Those files are logically separated
by specific purpose.
*   `\modules` - helpers for CMake to aid in loading external libraries
*   `dependencies.cmake` - contains information about project dependencies and
        'recipies' to build them alongisde the project
*   `flags.cmake` - specifies compilation flags
*   `ut.cmake` - provides facilities integrating project with GoogleTest unit
        test environment

### \src
#### \src\core
Contains core application logic and classes. This is core of the application
with general (abstract) classes where implementation-specific details should
be implemented.
*   `metric.hpp`
    *   `class Metric` - single metric, consisting of metadata, aggretage
        operation type and collection of `Sensor`s.
        *   Example metric:<br/>
            {**Id**: *MaxPower*, **Operation**: *MAX*,
                **Sensors**: *[CPU0_Power, CPU1_Power ..]*}
*   `report_mgr.hpp`
    *   `class ReportManager` - responsible for storing `Report`s.
            Its main responsibiltiies are making sure all `Report`s have unique
            name and comply with implementation-specific restrictions
            (`MaxReports` and `PollRateResolution`)
*   `report.hpp`
    *   `class Report` - groups `Metrics` into one named entity. Is able to
            monitor them per specified `ReportingType` (eg. periodically)
*   `sensor.hpp`
    *   `class Sensor` - abstract class with:
        *   abstract:
            *   unique `Id`
            *   `async_read` asynchronous IO function to retrieve sensor value
        *   generic:
            *   comparator (compare `Sensors` by `Id`)
            *   error functions for reporting IO errors to `Report`
    *   `class SensorCache` - makes makes sure that only one instance of
            `Sensor` object exists. All `Metrics` (and in effect `Sensors`)
            utilize the same object to interact with given sensor. This cache
            uses `Id` to check if given sensor was already available.
            `weak_ptr`s are used to store `Sensor` handles

#### \src\dbus
Contains D-Bus related definitions and wrappers. By principle no extra logic is
added apart from the logic of exposing proper D-Bus APIs. Exception is concrete
implementation fo `Sensor`, which allows to monitor values of sensors exposed
by `dbus-sensors` services.
*   `dbus.hpp` - contains basic types, constants and conversion functions
        (`core`<->`dbus`)
*   `report_mgr.hpp`
    *   `class ReportManager` - decorator of `Core::ReportManager` exposing it
            over D-Bus
    *   `class ReportFactory` - helper class with utility to construct
            `Core::Report` based on D-Bus parameters. It validates input and
            retrieves information about specified *D-Bus Sensor paths*
*   `report.hpp`
    *   `class Report` - decorator of `Core::Report` exposing it over D-Bus
*   `sensor.hpp`
    *   `class Sensor::Id` - concrete implementation of abstract class
            `Core::Sensor::Id`. It utilizes sensor path
            (*/xyz/openbmc_project/sensor/{type}/{name}* as unique key)
    *   `class Sensor` - concrete implementation of abstract class
            `Core::Sensor`. It utilizes sdbusplus async_read to retrieve sensor
            value on demand.

#### \src\utils

* `log.hpp`
  *   `class logger` - provides simple stream-based logger with varying
        loglevels and predefined formatting. Should be utilized to provide
        consistent and readable application logs
  *   `LOG_* macros` - wrappers for logger class. They come in two forms:
        *   `LOG_{level} << "message" << param << (...)` - classic log
        *   `LOG_{level}_T({variable/constant}) << "message" << param << (...)`
                \- tagged log, might be used to distinguish logs from different
                objects of the same class (by providing name/id)
* `dbus_interfaces.hpp`
  * `class DbusInterfaces` - provides utilities to construct and expose D-Bus interface
* `scoped_resource.hpp`
  * `class DbusScopedInterface` - encapsulates D-Bus interface object with custom action to be performed at destruction, to ensure proper interface cleanup

#### \src\main.cpp
Application entry point. Configures `logger`, instantiates `boost::asio`
and `sdbusplus` objects, starts `DBus::ReportManager`.

### \tests
Directory with unit tests. Compiled using common `CMakeLists.txt` to pin them
into `ctest` - CMake Unit Test engine. General rule of thumb is to implement
separate test files for separate classes.

# Design
## Relations
```ascii
   +------+
   |Report|
   +------+
     |
     |[monitors]
     |
     +----------------+
     |                |
     v                v
   +------+         +------+
   |Metric|  . . .  |Metric|  (Single, Avg, Sum..)
   +------+         +------+
     |
     |
     |[reads]
     |
     |
     v
   +------+
   |Sensor|  (Concrete sensor, eg CPU 1 Power)
   +------+
```
## Internal architecture
```ascii
+-------------------+ [creates] +--------------+  [reads]  +------------+
|Core::ReportManager+----------->Core::Report  |  +------->+Core::Sensor|
+--^------+---------+           |              |  |        +-----+------+
   |      |                     |   +------+   |  |              ^
   |      |                     |  +-------|   |  |              |
   |      |                     |  |Metric+-------+              |
   |      |                     |  +------+    |                 |
   |      |                     |              |                 |
   |      |                     +-+------------+                 |
   |      |                       |                              |
+--+------v---------+           +-v----------+             +-----+------+
|DBus::ReportManager|           |DBus::Report|             |DBus::Sensor|
+--+------+---------+           +-+----------+             +-----+------+
   ^      |                       |                              ^
   |      |                       |                              |
   |      |(Configuration)        |(Report data                  |(Sensor values
   |      |                       | exposed)                     | updates)
   |      |                       v                              |
+--+------v---------------D-Bus Interface------------------------+------+
```


# Flows
```ascii
       +---------------+     +--------+         +--------+      +--------+
       | ReportManager |     | Report |         | Metric |      | Sensor |
       +-------+-------+     +--+-----+         +------+-+      +------+-+
               |                |                      |               |
               |                |                      |               |
+--------------+----------------+--+-----------------------------------+
|  Creation flow (new report)      |                   |               |
+--------------+----------------+--+                   |               |
|              |                |                      |               |
|  add(Report) |                |                      |               |
+---------------+               |                      |               |
|              || validation    |                      |               |
|              ||               |                      |               |
|              +<               |                      |               |
|              |     Report(...)|                      |               |
|              +---------------->                      |               |
| return handle|                |                      |               |
+<-------------+                |                      |               |
|              |                |                      |               |
|              |                |                      |               |
+--------------+----------------+--+-----------------------------------+
|   Monitoring flow (periodic)     |                   |               |
+--------------+-------------------+                   |               |
|              |                |>                     |               |
|              |                ||timer tick           |               |
|              |                ||                     |               |
|              |                <+   foreach metric:   |               |
|              |                |       async_read()   |               |
|              |                +---------------------->               |
|              |                |                      |               |
|              |                |                      |               |
|              |                |                      |foreach sensor:|
|              |                |                      |   async_read()|
|              |                |                      +--------------->
|              |                |                      |--------------->
|              |                |                      +--------------->
|              |                |                      |               |
|              |                |                      |               |
|              |                |                      |               |
|              |                |                      |               |
|              |                |                      <---------------+
|              |                |                      <---------------|
|              |                |                      <---------------+
|              |                |                      |callback       |
|              |                |                      | (err, val)    |
|              |                |                      ++              |
|              |                |                      ||aggregate     |
|              |                |                      ||              |
|              |                |                      +<              |
|              |                <----------------------+               |
|              |                |  callback(err, val)  |               |
|              |                ++                     |               |
|              |                || update              |               |
|              |                ||  (timestamp,        |               |
|              |                ||   [values])         |               |
|              |                +<                     |               |
|              |                |                      |               |
|              |                |                      |               |
+--------------+----------------+----------------------+---------------+
```

# TBD
Below are list of known features which still lacks implementation
(as of WW11'20):
* `MaxReports` and `PollRateResolution` validation
  * `core\report_mgr.hpp`, `core\report.hpp`
* Only `Metrics` of `OperationType` `SINGLE` are supported
  * `core\metric.hpp`
* Only `Reports` of `ReportingType` `Periodic` is supported
  * `core\report.hpp`, `core\sensor.hpp`, `dbus\sensor.hpp`
  * see notes in `dbus\sensor.hpp` for clues about possible implementation
        for DBus sensor
* Thresholds and Triggers are not implemented at all
  * possibly new class needed, which will cooperate with `core\report_mgr.hpp`

project(
    'Telemetry',
    'cpp',
    meson_version: '>=0.54.3',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++17',
        # TODO: Without RTTI telemetry does not build using Boost 1.74.0
        # https://github.com/chriskohlhoff/asio/issues/533
        #'cpp_rtti=false',
        'warning_level=3',
        'werror=true',
        'b_lto=true',
    ],
    license: 'Apache-2.0',
)

cpp = meson.get_compiler('cpp')
add_project_arguments(
    cpp.get_supported_arguments([
        '-DBOOST_ASIO_DISABLE_THREADS',
        '-DBOOST_ALL_NO_LIB',
        '-DBOOST_SYSTEM_NO_DEPRECATED',
        '-DBOOST_ASIO_NO_DEPRECATED',
        '-DBOOST_NO_RTTI',
        '-DBOOST_NO_TYPEID',
        # TODO: Removed below arg after upgrade to Boost 1.75
        '-DBOOST_ALLOW_DEPRECATED_HEADERS',
        '-Wno-unused-parameter',
    ]),
    language: 'cpp'
)

boost = dependency(
    'boost',
    version: '>=1.74.0',
    required: false,
    modules: ['coroutine'])
assert(boost.found(),
       'Missing Boost 1.74.0 or higher, as WA you can set BOOST_ROOT ' +
       'environemt to point at boost build. To build a boost you can use ' +
       'script ./scripts/boost_build_1.74.0.sh')

phosphor_logging = dependency('phosphor-logging', required: false)
if not phosphor_logging.found()
    subproject('phosphor-logging', required: false)
    phosphor_logging = declare_dependency(
        include_directories: 'subprojects/phosphor-logging'
    )
endif

sdbusplus = dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep'])
systemd = dependency('systemd')

if cpp.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
else
    nlohmann_json = dependency('nlohmann_json',
                               fallback: ['nlohmann', 'nlohmann_json_dep'])
endif

add_project_arguments(
    '-DTELEMETRY_MAX_REPORTS=' + get_option('max-reports').to_string(),
    '-DTELEMETRY_MAX_READING_PARAMS=' +
        get_option('max-reading-parameters').to_string(),
    '-DTELEMETRY_MIN_INTERVAL=' + get_option('min-interval').to_string(),
    '-DTELEMETRY_MAX_TRIGGERS=' + get_option('max-triggers').to_string(),
    '-DTELEMETRY_MAX_REPORT_NAME_LENGTH=' +
        get_option('max-report-name-length').to_string(),
    language: 'cpp'
)

executable(
    'telemetry',
    [
        'src/discrete_threshold.cpp',
        'src/main.cpp',
        'src/metric.cpp',
        'src/numeric_threshold.cpp',
        'src/on_change_threshold.cpp',
        'src/persistent_json_storage.cpp',
        'src/report.cpp',
        'src/report_factory.cpp',
        'src/report_manager.cpp',
        'src/sensor.cpp',
        'src/sensor_cache.cpp',
        'src/trigger.cpp',
        'src/trigger_actions.cpp',
        'src/trigger_factory.cpp',
        'src/trigger_manager.cpp',
    ],
    dependencies: [
        boost,
        nlohmann_json,
        sdbusplus,
        phosphor_logging,
    ],
    include_directories: 'src',
    install: true,
    install_dir: get_option('prefix') / get_option('bindir'),
    pie: true,
)

configure_file(
    input: 'xyz.openbmc_project.Telemetry.service.in',
    output: 'xyz.openbmc_project.Telemetry.service',
    configuration: {
        'bindir': get_option('prefix') / get_option('bindir'),
    },
    install: true,
    install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir'),
)

if get_option('buildtest')
    subdir('tests')
endif

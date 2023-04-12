project(
    'Telemetry',
    'cpp',
    meson_version: '>=0.58.0',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++20',
        'warning_level=3',
        'werror=true',
        'b_lto=true',
    ],
    license: 'Apache-2.0',
)

cxx = meson.get_compiler('cpp')
add_project_arguments(
    cxx.get_supported_arguments([
        '-DBOOST_ASIO_DISABLE_THREADS',
        '-DBOOST_ALL_NO_LIB',
        '-DBOOST_SYSTEM_NO_DEPRECATED',
        '-DBOOST_ASIO_NO_DEPRECATED',
        '-DBOOST_NO_RTTI',
        '-DBOOST_NO_TYPEID',
        '-Wno-unused-parameter',
    ]),
    language: 'cpp'
)

boost_version = '>=1.79.0'
boost_modules = ['coroutine', 'context']
boost = dependency('boost',
    version: boost_version, 
    modules: boost_modules)

phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
systemd = dependency('systemd')

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
else
    nlohmann_json = dependency('nlohmann_json')
endif

add_project_arguments(
    '-DTELEMETRY_MAX_REPORTS=' + get_option('max-reports').to_string(),
    '-DTELEMETRY_MAX_READING_PARAMS=' +
        get_option('max-reading-parameters').to_string(),
    '-DTELEMETRY_MIN_INTERVAL=' + get_option('min-interval').to_string(),
    '-DTELEMETRY_MAX_TRIGGERS=' + get_option('max-triggers').to_string(),
    '-DTELEMETRY_MAX_DBUS_PATH_LENGTH=' +
        get_option('max-dbus-path-length').to_string(),
    '-DTELEMETRY_MAX_APPEND_LIMIT=' +
        get_option('max-append-limit').to_string(),
    '-DTELEMETRY_MAX_ID_NAME_LENGTH=' + get_option('max-id-name-length').to_string(),
    '-DTELEMETRY_MAX_PREFIX_LENGTH=' + get_option('max-prefix-length').to_string(),
    language: 'cpp'
)

executable(
    'telemetry',
    [
        'src/discrete_threshold.cpp',
        'src/main.cpp',
        'src/metric.cpp',
        'src/errors.cpp',
        'src/metrics/collection_data.cpp',
        'src/metrics/collection_function.cpp',
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
        'src/types/readings.cpp',
        'src/types/report_types.cpp',
        'src/utils/conversion_trigger.cpp',
        'src/utils/dbus_path_utils.cpp',
        'src/utils/make_id_name.cpp',
        'src/utils/messanger_service.cpp',
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

service_wants = ''
if get_option('service-wants').length() > 0
    service_wants = '\nWants=' + ' '.join(get_option('service-wants'))
endif

service_requires = ''
if get_option('service-requires').length() > 0
    service_requires = '\nRequires=' + ' '.join(get_option('service-requires'))
endif

service_before = ''
if get_option('service-before').length() > 0
    service_before = '\nBefore=' + ' '.join(get_option('service-before'))
endif

service_after = ''
if get_option('service-after').length() > 0
    service_after = ' ' + ' '.join(get_option('service-after'))
endif

configure_file(
    input: 'xyz.openbmc_project.Telemetry.service.in',
    output: 'xyz.openbmc_project.Telemetry.service',
    configuration: {
        'bindir': get_option('prefix') / get_option('bindir'),
        'wants': service_wants,
        'requires': service_requires,
        'before': service_before,
        'after': service_after
    },
    install: true,
    install_dir: systemd.get_variable('systemdsystemunitdir'),
)

if get_option('buildtest')
    subdir('tests')
endif

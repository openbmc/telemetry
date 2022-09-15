gtest_dep = dependency('gtest', main: false, required: false, version: '>=1.11.0')
gmock_dep = dependency('gmock', required: false, version: '>=1.11.0')
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest')
    gtest_dep = gtest_proj.dependency('gtest')
    gmock_dep = gtest_proj.dependency('gmock')
endif

test(
    'telemetry-ut',
    executable(
        'telemetry-ut',
        [
            '../src/discrete_threshold.cpp',
            '../src/metric.cpp',
            '../src/metrics/collection_data.cpp',
            '../src/metrics/collection_function.cpp',
            '../src/numeric_threshold.cpp',
            '../src/on_change_threshold.cpp',
            '../src/persistent_json_storage.cpp',
            '../src/report.cpp',
            '../src/report_factory.cpp',
            '../src/report_manager.cpp',
            '../src/sensor.cpp',
            '../src/sensor_cache.cpp',
            '../src/trigger.cpp',
            '../src/trigger_actions.cpp',
            '../src/trigger_factory.cpp',
            '../src/trigger_manager.cpp',
            '../src/types/readings.cpp',
            '../src/types/report_types.cpp',
            '../src/utils/conversion_trigger.cpp',
            '../src/utils/dbus_path_utils.cpp',
            '../src/utils/make_id_name.cpp',
            '../src/utils/messanger_service.cpp',
            'src/dbus_environment.cpp',
            'src/main.cpp',
            'src/stubs/dbus_sensor_object.cpp',
            'src/test_conversion.cpp',
            'src/test_detached_timer.cpp',
            'src/test_discrete_threshold.cpp',
            'src/test_ensure.cpp',
            'src/test_labeled_tuple.cpp',
            'src/test_make_id_name.cpp',
            'src/test_metric.cpp',
            'src/test_numeric_threshold.cpp',
            'src/test_on_change_threshold.cpp',
            'src/test_path_append.cpp',
            'src/test_persistent_json_storage.cpp',
            'src/test_report.cpp',
            'src/test_report_manager.cpp',
            'src/test_sensor.cpp',
            'src/test_sensor_cache.cpp',
            'src/test_transform.cpp',
            'src/test_trigger.cpp',
            'src/test_trigger_actions.cpp',
            'src/test_trigger_manager.cpp',
            'src/test_unique_call.cpp',
            'src/utils/generate_unique_mock_id.cpp',
            'src/utils/string_utils.cpp',
        ],
        dependencies: [
            boost,
            gmock_dep,
            gtest_dep,
            nlohmann_json,
            phosphor_logging,
            sdbusplus,
        ],
        include_directories: ['../src', 'src'],
        cpp_args: '-fno-lto'
    ),
    timeout: 120,
)

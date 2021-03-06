gtest_dep = dependency('gtest', main: false, required: false, version: '>=1.10.0')
gmock_dep = dependency('gmock', required: false, version: '>=1.10.0')
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
            'src/dbus_environment.cpp',
            'src/main.cpp',
            'src/stubs/dbus_sensor_object.cpp',
            'src/test_conversion.cpp',
            'src/test_detached_timer.cpp',
            'src/test_discrete_threshold.cpp',
            'src/test_metric.cpp',
            'src/test_numeric_threshold.cpp',
            'src/test_on_change_threshold.cpp',
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
        ],
        dependencies: [
            boost,
            gmock_dep,
            gtest_dep,
            nlohmann_json,
            phosphor_logging,
            sdbusplus,
        ],
        include_directories: ['../src', 'src']
    ),
    timeout: 120,
)

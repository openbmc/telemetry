gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
gmock_dep = dependency('gmock', disabler: true, required: false)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
  else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled'
        )
  endif
endif

test(
    'monitoring-service-ut',
    executable(
        'monitoring-service-ut',
        [
            '../src/persistent_json_storage.cpp',
            '../src/sensor_cache.cpp',
            'src/test_persistent_json_storage.cpp',
            'src/test_sensor_cache.cpp',
            'src/utils/generate_unique_mock_id.cpp',
        ],
        dependencies: [
            boost,
            gmock_dep,
            gtest_dep,
            nlohmann_json,
            sdbusplus,
        ],
        include_directories: ['../src', 'src']
    )
)

project(
    'Telemetry',
    'cpp',
    meson_version: '>=0.55.0',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++17',
        'cpp_rtti=false',
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
        '-Wno-unused-parameter',
        '-DTELEMETRY_ENABLE_LOGS_TIMESTAMP=' + get_option('logtimestamp').to_string(),
        '-DTELEMETRY_ENABLE_LOGS=' + get_option('loglevel').to_string(),
    ]),
    language: 'cpp'
)

boost = dependency('boost', required: false)
if not boost.found()
     subproject('boost', required: true)
     boost = declare_dependency(
         include_directories: 'subprojects/boost_1_73_0',
     )
endif

sdbusplus = dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep'])

executable(
    'telemetry',
    [
        'src/main.cpp',
        'src/report.cpp',
        'src/report_manager.cpp',
    ],
    dependencies: [
        boost,
        sdbusplus,
    ],
    include_directories: 'src',
    install: true,
    install_dir: get_option('prefix') / get_option('bindir'),
    pie: true,
)

systemd = dependency('systemd')

configure_file(
    input: 'xyz.openbmc_project.Telemetry.service.in',
    output: 'xyz.openbmc_project.Telemetry.service',
    configuration: {
        'bindir': get_option('prefix') / get_option('bindir'),
    },
    install: true,
    install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir'),
)

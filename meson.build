project(
    'MonitoringService',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17',
        'buildtype=debugoptimized'
    ],
    license: 'Apache-2.0',
)

boost_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ASIO_NO_DEPRECATED',
    '-DBOOST_NO_RTTI',
    '-DBOOST_NO_TYPEID',
]

cpp_args = [
    '-Wno-unused-parameter',
    '-flto',
    '-fno-rtti',
    '-fstack-protector-strong',
    '-fpic',
    '-DMS_ENABLE_LOGS_TIMESTAMP=' + get_option('logtimestamp').to_string(),
    '-DMS_ENABLE_LOGS=' + get_option('loglevel').to_string(),
]

boost = dependency('boost', required: false)
if not boost.found()
     subproject('boost', required: true)
     boost = declare_dependency(
         include_directories: 'subprojects/boost_1_73_0',
     )
endif

sdbusplus = dependency('sdbusplus', static: true, fallback: ['sdbusplus', 'sdbusplus_dep'])

src_files = [
    'src/main.cpp',
    'src/report_manager.cpp',
]

executable(
    'monitoring-service',
    src_files,
    cpp_args: cpp_args + boost_args,
    dependencies: [
        boost,
        sdbusplus,
    ],
    include_directories: 'src',
    install: true,
    install_dir: get_option('bindir'),
    pie: true,
)

systemd = dependency('systemd')

configure_file(
    input: 'xyz.openbmc_project.MonitoringService.service.in',
    output: 'xyz.openbmc_project.MonitoringService.service',
    install: true,
    install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir'),
)

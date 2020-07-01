cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

project (monitoring-service CXX)

# General options
option (ENABLE_UT "Enable unit tests" ON)
option (ENABLE_UT_EXECUTION "Enable UT execution as part of build process" ON)
option (ENABLE_LOGS "Enable logging" ON)

# Compilation options
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Yocto integration
option(YOCTO_DEPENDENCIES "Use YOCTO dependencies system" OFF)

# Build dependencies
include(cmake/dependencies.cmake)

# Compilation flags
include(cmake/flags.cmake)

# Build unit tests
if (${ENABLE_UT})
    include(cmake/ut.cmake)
    add_subdirectory(tests)
endif (${ENABLE_UT})

# Include directories
include_directories (src)
include_directories (src/utils)

# Source files
set (
    SRC_FILES
    src/main.cpp
    src/core/persistent_storage.cpp
)

# Application
add_executable (monitoring-service ${SRC_FILES})

if (NOT ${YOCTO_DEPENDENCIES})
    add_dependencies (monitoring-service sdbusplus-project)
    add_dependencies (monitoring-service nlohmann-json-project)
endif ()

target_link_libraries (monitoring-service systemd)
target_link_libraries (monitoring-service stdc++fs)
target_link_libraries (monitoring-service sdbusplus)
target_link_libraries (monitoring-service boost_date_time)
target_link_libraries (monitoring-service boost_coroutine)

target_compile_definitions (
    monitoring-service
    PRIVATE
    $<$<BOOL:${ENABLE_LOGS}>: -DENABLE_LOGS>
    )

install (
    TARGETS monitoring-service
    DESTINATION sbin)

pkg_get_variable (SYSTEMD_SYSTEMUNITDIR systemd systemdsystemunitdir)

install(
    FILES ${PROJECT_SOURCE_DIR}/xyz.openbmc_project.MonitoringService.service
    DESTINATION ${SYSTEMD_SYSTEMUNITDIR})

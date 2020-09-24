cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

project (monitoring-service CXX)

# General options
option (MS_ENABLE_LOGS "Enable logging" OFF)
option (MS_ENABLE_LOGS_TIMESTAMP "Add timestamp to log messages" OFF)

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

# Include directories
include_directories (src)
include_directories (src/utils)

# Source files
set (
    SRC_FILES
    src/main.cpp
    src/report_manager.cpp
)

# Application
add_executable (monitoring-service ${SRC_FILES})

target_link_libraries (monitoring-service systemd)
target_link_libraries (monitoring-service sdbusplus)

target_compile_definitions (
    monitoring-service
    PRIVATE
    -DMS_ENABLE_LOGS=$<BOOL:${MS_ENABLE_LOGS}>
    -DMS_ENABLE_LOGS_TIMESTAMP=$<BOOL:${MS_ENABLE_LOGS_TIMESTAMP}>
    )

install (
    TARGETS monitoring-service
    DESTINATION bin)

pkg_get_variable (SYSTEMD_SYSTEMUNITDIR systemd systemdsystemunitdir)

install(
    FILES ${PROJECT_SOURCE_DIR}/xyz.openbmc_project.MonitoringService.service
    DESTINATION ${SYSTEMD_SYSTEMUNITDIR})

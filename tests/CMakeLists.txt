cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

include (CTest)

enable_testing ()

find_package (Threads REQUIRED)

add_executable (monitoring-service-ut
    src/main.cpp
    ../src/persistent_json_storage.cpp
    ../src/sensor_cache.cpp
    src/test_persistent_json_storage.cpp
    src/test_sensor_cache.cpp
    src/utils/generate_unique_mock_id.cpp
)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src/utils)

FIND_PATH(GTEST_INCLUDE
    NAMES gtest/gtest.h
)
FIND_PATH(GMOCK_INCLUDE
    NAMES gmock/gmock.h
)

include_directories (${GTEST_INCLUDE})
include_directories (${GMOCK_INCLUDE})

FIND_LIBRARY(GTEST_LIBRARIES
    NAMES gtest
)
FIND_LIBRARY(GMOCK_LIBRARIES
    NAMES gmock
)

target_link_libraries(monitoring-service-ut
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
target_link_libraries (monitoring-service-ut systemd)
target_link_libraries (monitoring-service-ut stdc++fs)
target_link_libraries (monitoring-service-ut sdbusplus)
target_link_libraries (monitoring-service-ut boost_coroutine)

target_compile_definitions (
    monitoring-service-ut
    PRIVATE
    -DMS_ENABLE_LOGS=$<BOOL:${MS_ENABLE_LOGS}>
    -DMS_ENABLE_LOGS_TIMESTAMP=$<BOOL:${MS_ENABLE_LOGS_TIMESTAMP}>
    )

set (MONITORING_SERVICE_UT "monitoring-service-ut")

add_test(
    NAME ${MONITORING_SERVICE_UT}
    COMMAND ${MONITORING_SERVICE_UT}
)


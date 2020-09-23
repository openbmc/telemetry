cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

include (CTest)

enable_testing ()

find_package (Threads REQUIRED)
find_package (GTest 1.10 REQUIRED)
find_package (GMock 1.10 REQUIRED)

add_executable (monitoring-service-ut
    src/main.cpp
    src/test_persistent_json_storage.cpp
    src/test_sensor_cache.cpp
    src/mocks/json_storage.cpp
    ../src/persistent_json_storage.cpp
)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src/utils)

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


cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

add_executable (ut_core
    ../src/core/persistent_json_storage.cpp
    dbus/dbus_ut.cpp
    dbus/report_ut.cpp
    core/report_ut.cpp
    core/persistent_json_storage_ut.cpp
    utils/signals_ut.cpp
    main.cpp
)

if (NOT ${YOCTO_DEPENDENCIES})
    add_dependencies (ut_core sdbusplus-project)
    add_dependencies (ut_core nlohmann-json-project)
endif ()

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../src/utils)

target_link_libraries(ut_core
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
target_link_libraries (ut_core systemd)
target_link_libraries (ut_core stdc++fs)
target_link_libraries (ut_core sdbusplus)
target_link_libraries (ut_core boost_date_time)
target_link_libraries (ut_core boost_coroutine)

set (UT_CORE "ut_core")

add_test(
    NAME ${UT_CORE}
    COMMAND ${UT_CORE}
)

if (${ENABLE_UT_EXECUTION})
    add_custom_command(
        TARGET ${UT_CORE}
        COMMENT "Run tests for core components"
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        COMMAND GTEST_COLOR=1 ${CMAKE_CTEST_COMMAND} --output-on-failure
    )
endif()

cmake_minimum_required (VERSION 3.5)

include (ExternalProject)

if (NOT ${YOCTO_DEPENDENCIES})
    externalproject_add (
    ####### sdbusplus #######
        sdbusplus-project
        PREFIX "${CMAKE_BINARY_DIR}/sdbusplus-project"
        GIT_REPOSITORY "https://github.com/openbmc/sdbusplus.git"
        GIT_TAG 396ba143454031f803ad18b5136c331a4b90fff5
        SOURCE_DIR "${CMAKE_BINARY_DIR}/sdbusplus-src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/sdbusplus-build"
        CONFIGURE_COMMAND cd "${CMAKE_BINARY_DIR}/sdbusplus-src" && ./bootstrap.sh && ./configure --enable-transaction
        BUILD_COMMAND cd "${CMAKE_BINARY_DIR}/sdbusplus-src" && make -j "libsdbusplus.la"
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
        LOG_DOWNLOAD ON
    )

    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/sdbusplus-src)
    link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/.libs)

    ####### nlohmann-json #######
    externalproject_add (
        nlohmann-json-project
        PREFIX "${CMAKE_BINARY_DIR}/nlohmann-json-project"
        GIT_REPOSITORY "https://github.com/nlohmann/json.git"
        GIT_TAG e7452d87783fbf6e9d320d515675e26dfd1271c5
        SOURCE_DIR "${CMAKE_BINARY_DIR}/nlohmann-json-src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/nlohmann-json-build"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    include_directories (${CMAKE_BINARY_DIR}/nlohmann-json-src/include)
endif ()

####### Boost #######
find_package (Boost 1.71 REQUIRED)
include_directories (SYSTEM ${BOOST_SRC_DIR})

# Verbose debugging of asio internals
# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

# Main config options
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DBOOST_BEAST_USE_STD_STRING_VIEW)
add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)

# Deprecation settings
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
message (BOOST_VERSION = ${Boost_VERSION})
if ("${Boost_VERSION}" STREQUAL "107100")
    add_definitions (-DBOOST_ASIO_NO_DEPRECATED)
endif ()

# ASIO uses deprecated coroutine library
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

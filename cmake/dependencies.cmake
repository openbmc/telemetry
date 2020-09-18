if (NOT ${YOCTO_DEPENDENCIES})
    configure_file (CMakeLists.txt.in external-projects/CMakeLists.txt)

    execute_process (COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/external-projects)
    execute_process (COMMAND ${CMAKE_COMMAND} --build .
                     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/external-projects)

    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/dependencies/include)
    link_directories (${CMAKE_BINARY_DIR}/dependencies/lib)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/dependencies)
    set(BOOST_INCLUDEDIR ${CMAKE_BINARY_DIR}/dependencies/include)
    set(BOOST_LIBRARYDIR ${CMAKE_BINARY_DIR}/dependencies/lib)
endif ()

find_package(PkgConfig REQUIRED)

####### Boost #######
find_package (Boost 1.73.0 REQUIRED)
include_directories (SYSTEM ${BOOST_SRC_DIR})

message (BOOST_VERSION = ${Boost_VERSION})
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ASIO_NO_DEPRECATED)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)

# Verbose debugging of asio internals
# add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

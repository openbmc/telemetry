cmake_minimum_required (VERSION 3.5)
include (CTest)

enable_testing ()

#TODO: externalproject for googlemock and googletest
find_package (Threads REQUIRED)
find_package (GTest 1.10 REQUIRED)
find_package (GMock 1.10 REQUIRED)
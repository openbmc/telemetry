set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -Werror \
    -Wall \
    -Wextra \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wcast-align \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wconversion \
    -Wsign-conversion \
    -Wnull-dereference \
    -Wduplicated-cond \
    -Wduplicated-branches \
    -Wlogical-op \
    -Wdouble-promotion \
    -Wformat=2 \
    -Wno-unused-parameter \
    -Os \
    -flto \
    -fno-rtti \
")

# Security-oriented compilation flags
set (SECURITY_FLAGS
    "-fstack-protector-strong \
    -fPIE \
    -fPIC \
    -D_FORTIFY_SOURCE=2 \
    -Wformat \
    -Wformat-security \
")

# Set __FILENAME__ as absolute path of __FILE__
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
    ${SECURITY_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} \
    ${SECURITY_FLAGS}")
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} \
    ${SECURITY_FLAGS}")

cmake_minimum_required(VERSION 3.0)
project(zmonkey)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g -DALLOW_EXPERIMENTAL_API -mavx2  -march=native")

find_package(PkgConfig REQUIRED)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(DPDK "libdpdk")
    if (DPDK_FOUND)
        message(STATUS "found dpdk via pkg-config")
        add_definitions(${DPDK_STATIC_CFLAGS})
        set(MYDPDK_LIBRARIES ${DPDK_STATIC_LDFLAGS})
        include_directories(${DPDK_INCLUDE_DIR})
        add_definitions(-DHAVE_DPDK)
    endif ()
endif ()



set(SRCS main.c portinit.c monkey.c cli_parser.c)

add_executable(zmonkey ${SRCS})
target_link_libraries(zmonkey ${MYDPDK_LIBRARIES})

file(MAKE_DIRECTORY build)
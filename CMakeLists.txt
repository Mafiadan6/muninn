cmake_minimum_required(VERSION 3.22)
project(muninn LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

find_path(CRYPTOPP_INCLUDE_DIR cryptlib.h
    PATHS /usr/local/include /usr/include
    PATH_SUFFIXES cryptopp crypto++
)

set(_OLD_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

find_library(CRYPTOPP_STATIC_LIB
    NAMES cryptopp libcryptopp
    PATHS /usr/local/lib /usr/local/lib64 /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${_OLD_CMAKE_FIND_LIBRARY_SUFFIXES})

if (CRYPTOPP_INCLUDE_DIR AND CRYPTOPP_STATIC_LIB AND CRYPTOPP_STATIC_LIB MATCHES "\\.a$")
    set(CRYPTOPP_USE_STATIC ON)
else()
    set(CRYPTOPP_USE_STATIC OFF)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRYPTOPP_ALT QUIET libcryptopp)
    if (CRYPTOPP_ALT_FOUND)
        set(CRYPTOPP_INCLUDE_DIRS ${CRYPTOPP_ALT_INCLUDE_DIRS})
        set(CRYPTOPP_LIBRARIES ${CRYPTOPP_ALT_LIBRARIES})
        set(CRYPTOPP_FOUND ON)
    else()
        pkg_check_modules(CRYPTOPP REQUIRED libcrypto++)
    endif()
endif()

file(STRINGS "${CMAKE_SOURCE_DIR}/src/muninn.cpp" MUNINN_VERSION_LINE REGEX "#define MUNINN_VERSION")
string(REGEX MATCH "\"[^\"]*\"" MUNINN_VERSION_RAW "${MUNINN_VERSION_LINE}")
string(REPLACE "\"" "" MUNINN_VERSION_CLEAN "${MUNINN_VERSION_RAW}")
string(REGEX MATCH "^[0-9]+" MUNINN_SOVERSION "${MUNINN_VERSION_CLEAN}")

add_library(lz4_lib STATIC
    lib/lz4/lz4.c
    lib/lz4/lz4hc.c
    lib/lz4/lz4frame.c
    lib/lz4/xxhash.c
)
target_include_directories(lz4_lib PUBLIC lib/lz4)

add_subdirectory(src)

add_library(muninn_static STATIC ${MUNINN_CORE_SOURCES})
target_include_directories(muninn_static PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_include_directories(muninn_static PRIVATE ${CMAKE_SOURCE_DIR}/src ${LIBUSB_INCLUDE_DIRS})

add_library(muninn_shared SHARED ${MUNINN_CORE_SOURCES})
set_target_properties(muninn_shared PROPERTIES 
    OUTPUT_NAME "muninn" 
    VERSION "${MUNINN_VERSION_CLEAN}" 
    SOVERSION "${MUNINN_SOVERSION}"
)
target_include_directories(muninn_shared PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_include_directories(muninn_shared PRIVATE ${CMAKE_SOURCE_DIR}/src ${LIBUSB_INCLUDE_DIRS})

foreach(target muninn_static muninn_shared)
    if (CRYPTOPP_USE_STATIC)
        target_include_directories(${target} PRIVATE ${CRYPTOPP_INCLUDE_DIR})
        target_link_libraries(${target} PRIVATE ${CRYPTOPP_STATIC_LIB})
    else()
        target_include_directories(${target} PRIVATE ${CRYPTOPP_INCLUDE_DIRS})
        target_link_libraries(${target} PRIVATE ${CRYPTOPP_LIBRARIES})
    endif()
    target_link_libraries(${target} PRIVATE ${LIBUSB_LIBRARIES} lz4_lib)
endforeach()

add_executable(muninn src/cli/main.cpp)
set_target_properties(muninn PROPERTIES OUTPUT_NAME "muninn")
target_include_directories(muninn PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(muninn PRIVATE muninn_static -static-libgcc -static-libstdc++)

# Optional GUI build
option(MUNINN_BUILD_GUI "Build Qt6 GUI target" ON)
set(MUNINN_ALL_TARGETS muninn_static muninn_shared muninn)

# GUI Target
if(MUNINN_BUILD_GUI)
    find_package(Qt6 COMPONENTS Widgets REQUIRED)
    add_executable(muninn-gui src/gui/main.cpp)
    set_target_properties(muninn-gui PROPERTIES
        OUTPUT_NAME "muninn-gui"
        AUTOMOC ON
    )
    target_include_directories(muninn-gui PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(muninn-gui PRIVATE muninn_static Qt6::Widgets)
    if(NOT MSVC)
        target_link_options(muninn-gui PRIVATE "-static-libgcc" "-static-libstdc++")
    endif()
    list(APPEND MUNINN_ALL_TARGETS muninn-gui)
endif()

# Tests
option(MUNINN_BUILD_TESTS "Build test suite" ON)

if(MUNINN_BUILD_TESTS)
    enable_testing()
    
    set(MUNINN_TEST_SOURCES
        tests/main.cpp
        tests/test_framework.h
        tests/test_thor_protocol.cpp
        tests/test_odin_types.cpp
        tests/test_logger.cpp
    )
    
    add_executable(muninn_tests ${MUNINN_TEST_SOURCES})
    set_target_properties(muninn_tests PROPERTIES OUTPUT_NAME "muninn_tests")
    target_include_directories(muninn_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_include_directories(muninn_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(muninn_tests PRIVATE -Wall -Wextra -Wno-cast-function-type)
    
    add_test(NAME muninn_tests COMMAND $<TARGET_FILE:muninn_tests>)
    list(APPEND MUNINN_ALL_TARGETS muninn_tests)
endif()

# Fuzz targets are built in .clusterfuzzlite/build.sh
# The muninn_fuzz library provides PIT parsing for fuzzing

set(MUNINN_FUZZ_SOURCES
    src/fuzz_utils.cpp
    lib/lz4/lz4.c
    lib/lz4/lz4hc.c
    lib/lz4/lz4frame.c
    lib/lz4/xxhash.c
)

add_library(muninn_fuzz STATIC ${MUNINN_FUZZ_SOURCES})
target_include_directories(muninn_fuzz PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(muninn_fuzz PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_include_directories(muninn_fuzz PUBLIC ${CMAKE_SOURCE_DIR}/lib)

foreach(target ${MUNINN_ALL_TARGETS})
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-cast-function-type)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -fstack-protector-strong -D_FORTIFY_SOURCE=2)
        target_link_options(${target} PRIVATE "SHELL:-z relro" "SHELL:-z now")
        if(target STREQUAL "muninn")
            target_compile_options(${target} PRIVATE -fPIE)
            target_link_options(${target} PRIVATE "-pie")
        endif()
    endif()
endforeach()
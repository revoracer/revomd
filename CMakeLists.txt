# revomd — normalized market-data schema. The leaf every other module depends
# on: it defines the vocabulary and depends on nothing itself (header-only,
# STL only). Deliberately owned by neither a producer nor a consumer — see
# include/revomd/types.h for the rationale.

cmake_minimum_required(VERSION 3.23)

project(
    revomd
    VERSION 0.1.0
    LANGUAGES CXX
)

option(REVOMD_BUILD_TESTS "Build revomd C++ tests" ON)
option(REVOMD_ENABLE_INSTALL "Generate revomd install and package export rules" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

add_library(revomd INTERFACE)
add_library(revomd::revomd ALIAS revomd)

file(GLOB_RECURSE REVOMD_PUBLIC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/revomd/*.h"
)

target_sources(revomd
    INTERFACE
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES ${REVOMD_PUBLIC_HEADERS}
)

target_include_directories(revomd
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(revomd INTERFACE cxx_std_20)

if(REVOMD_ENABLE_INSTALL)
    install(TARGETS revomd
        EXPORT revomdTargets
        FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    install(EXPORT revomdTargets
        NAMESPACE revomd::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/revomd"
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/revomdConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/revomdConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/revomd"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/revomdConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/revomdConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/revomdConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/revomd"
    )
endif()

if(REVOMD_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        if(NOT TARGET Catch2::Catch2WithMain)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/CMakeLists.txt")
                set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
                set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
                set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
                add_subdirectory(external/Catch2 EXCLUDE_FROM_ALL)
            else()
                find_package(Catch2 CONFIG QUIET)
            endif()
        endif()

        if(NOT TARGET Catch2::Catch2WithMain)
            message(FATAL_ERROR
                "Missing dependency Catch2 (Catch2::Catch2WithMain). "
                "Install it as a CMake package, or initialize the external/Catch2 submodule.")
        endif()

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/extras/Catch.cmake")
            list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/extras")
        endif()

        file(GLOB REVOMD_TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
        )

        if(REVOMD_TEST_SOURCES)
            add_executable(revomd_tests ${REVOMD_TEST_SOURCES})
            target_link_libraries(revomd_tests
                PRIVATE
                    revomd::revomd
                    Catch2::Catch2WithMain
            )
            if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
                target_compile_options(revomd_tests PRIVATE -Wall -Wextra -Wpedantic)
            endif()

            include(Catch OPTIONAL RESULT_VARIABLE REVOMD_CATCH_MODULE)
            if(REVOMD_CATCH_MODULE)
                catch_discover_tests(revomd_tests)
            else()
                add_test(NAME revomd_tests COMMAND revomd_tests)
            endif()
        endif()
    endif()
endif()

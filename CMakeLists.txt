cmake_minimum_required(VERSION 3.21)

# Detect if Sub0Pub is being included inside another project
if(DEFINED PROJECT_NAME)
  set(IS_SUBPROJECT ON)
endif()

project(Sub0Pub VERSION 2.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

option(SUB0PUB_BUILD_TESTING "Build unit-tests" ON)
option(SUB0PUB_BUILD_EXAMPLES "Build examples" OFF)

# Warnings for Sub0Pub's own tests and examples. Not a usage requirement of the Sub0Pub target: a consumer's
# warning level is its own choice (the headers are kept clean at these levels by the builds below).
if(MSVC)
    add_compile_options(/W4 /Zc:preprocessor)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(SUB0PUB_BUILD_TESTING AND NOT IS_SUBPROJECT)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SUB0PUB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Sub0Pub as header only target
add_library(Sub0Pub INTERFACE)
target_compile_features(Sub0Pub INTERFACE cxx_std_23)
add_library(Sub0Pub::Sub0Pub ALIAS Sub0Pub)

target_include_directories(Sub0Pub
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_sources( Sub0Pub 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/sub0pub/sub0pub.hpp>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sub0pub/sub0pub.hpp>
)

# Install project if not included by add_subdirectory(Sub0Pub) from another project
if (NOT IS_SUBPROJECT)

    include(CMakePackageConfigHelpers)
    
    set(SUB0PUB_CMAKE_CONFIG_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Sub0Pub")
    
    configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/cmake/Sub0PubConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Sub0PubConfig.cmake"
        INSTALL_DESTINATION
          "${SUB0PUB_CMAKE_CONFIG_DESTINATION}"
    )

    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/Sub0PubConfigVersion.cmake"
        COMPATIBILITY 
            SameMajorVersion
        ARCH_INDEPENDENT # header-only: usable from any target architecture
    )

    install(
        TARGETS
            Sub0Pub
        EXPORT
            Sub0PubTargets
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}
    )

    install(
        EXPORT
            Sub0PubTargets
        NAMESPACE
            Sub0Pub::
        DESTINATION
            ${SUB0PUB_CMAKE_CONFIG_DESTINATION}
    )
    
    install(
        FILES 
            "${PROJECT_BINARY_DIR}/Sub0PubConfigVersion.cmake"
            "${PROJECT_BINARY_DIR}/Sub0PubConfig.cmake"
        DESTINATION 
            "${SUB0PUB_CMAKE_CONFIG_DESTINATION}"
    )
    
    # Install include sources
    install(
        DIRECTORY
            "include/"
        DESTINATION
            "${CMAKE_INSTALL_INCLUDEDIR}"
    )
    
endif()
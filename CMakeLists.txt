cmake_minimum_required(VERSION 3.10)
project(akerror LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(AKERR_USE_STDLIB 1 CACHE BOOL "Use the C standard library")
set(akerror_install_cmakedir "${CMAKE_INSTALL_LIBDIR}/cmake/akerror")

find_package(PkgConfig REQUIRED)
add_library(akerror STATIC
    src/error.c
)

target_compile_definitions(akerror
    PUBLIC AKERR_USE_STDLIB=${AKERR_USE_STDLIB}
)

add_executable(test_err_catch tests/err_catch.c)
add_executable(test_err_cleanup tests/err_cleanup.c)
add_executable(test_err_trace tests/err_trace.c)
add_test(NAME err_catch COMMAND test_err_catch)
add_test(NAME err_cleanup COMMAND test_err_cleanup)
add_test(NAME err_trace COMMAND test_err_trace)

# Specify include directories for the library's headers (if applicable)
target_include_directories(akerror PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)
target_link_libraries(test_err_catch PRIVATE akerror)
target_link_libraries(test_err_cleanup PRIVATE akerror)
target_link_libraries(test_err_trace PRIVATE akerror)

set(main_lib_dest "lib/my_library-${MY_LIBRARY_VERSION}")
install(TARGETS akerror EXPORT akerror DESTINATION "lib/")
install(FILES "include/akerror.h" DESTINATION "include/")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/akerror.pc DESTINATION "lib/pkgconfig/")


install(EXPORT akerror
    FILE akerrorTargets.cmake
    NAMESPACE akerror::
    DESTINATION ${akerror_install_cmakedir}
)

configure_package_config_file(
    cmake/akerror.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/akerrorConfig.cmake"
    INSTALL_DESTINATION ${akerror_install_cmakedir}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/akerrorConfig.cmake"
    DESTINATION ${akerror_install_cmakedir}
)

# pkgconfig
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(libdir      "\${exec_prefix}/lib")
set(includedir  "\${prefix}/include")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/akerror.pc.in ${CMAKE_CURRENT_BINARY_DIR}/akerror.pc @ONLY)

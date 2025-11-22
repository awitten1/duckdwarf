cmake_minimum_required(VERSION 3.5)

# Set extension name here
set(TARGET_NAME dwarf)

# DuckDB's extension distribution supports vcpkg. As such, dependencies can be added in ./vcpkg.json and then
# used in cmake with find_package. Feel free to remove or replace with other dependencies.
# Note that it should also be removed from vcpkg.json to prevent needlessly installing it..
find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(src/include)

set(LIBDWARF_INSTALL_DIR libdwarf-install CACHE STRING "libdwarf installation directory")
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/${LIBDWARF_INSTALL_DIR}" ${CMAKE_PREFIX_PATH})
pkg_check_modules(LIBDWARF REQUIRED libdwarf)

set(EXTENSION_SOURCES src/dwarf_extension.cpp)
set(EXTENSION_SOURCES src/dwarf.cpp)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

message(STATUS "LIBDWARF_STATIC_LDFLAGS = ${LIBDWARF_STATIC_LDFLAGS}")
add_executable(main main.cpp)
target_include_directories(main PRIVATE src/include)
target_link_libraries(main ${LIBDWARF_STATIC_LDFLAGS})
target_include_directories(main PRIVATE ${LIBDWARF_STATIC_INCLUDE_DIRS})
target_compile_definitions(main PRIVATE LIBDWARF_STATIC)


target_compile_definitions(${EXTENSION_NAME} PRIVATE LIBDWARF_STATIC)
target_link_libraries(${EXTENSION_NAME} ${LIBDWARF_STATIC_LIBRARIES})
target_include_directories(${EXTENSION_NAME} PRIVATE ${LIBDWARF_STATIC_INCLUDE_DIRS})

# Link OpenSSL in both the static library as the loadable extension
target_link_libraries(${EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(${LOADABLE_EXTENSION_NAME} OpenSSL::SSL OpenSSL::Crypto)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")

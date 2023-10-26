cmake_minimum_required(VERSION 3.5.0)
set(TARGET_NAME mysql_scanner)
project(${TARGET_NAME})

find_package(libmysql REQUIRED)

include_directories(${CMAKE_BINARY_DIR}/vcpkg_installed/arm64-osx/include/mysql)

add_subdirectory(src)

set(PARAMETERS "-no-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${ALL_OBJECT_FILES})

target_include_directories(${TARGET_NAME}_loadable_extension
                           PRIVATE include ${CMAKE_BINARY_DIR}/vcpkg_installed/arm64-osx/include/mysql)

target_link_libraries(${TARGET_NAME}_loadable_extension ${MYSQL_LIBRARIES})

if(WIN32)
  target_link_libraries(${TARGET_NAME}_loadable_extension wsock32 ws2_32
                        wldap32 secur32 crypt32)
endif()

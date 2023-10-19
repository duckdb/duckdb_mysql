cmake_minimum_required(VERSION 3.5.0)
set(TARGET_NAME mysql_scanner)
project(${TARGET_NAME})

set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(OpenSSL REQUIRED)

link_directories(/opt/homebrew/Cellar/mysql/8.1.0/lib)
#add_library()
#-L -lmysqlclient -lz  -lzstd -L/opt/homebrew/lib -lssl -lcrypto -lresolv
include_directories(mysql/include)

add_subdirectory(src)

if(EXISTS "mysql")
else()
  execute_process(
    COMMAND sh mysqlconfigure
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

set(PARAMETERS "-no-warnings")
build_loadable_extension(${TARGET_NAME} ${PARAMETERS} ${ALL_OBJECT_FILES}
                         ${LIBPG_SOURCES_FULLPATH})

target_include_directories(
  ${TARGET_NAME}_loadable_extension
  PRIVATE include mysql/include ${OPENSSL_INCLUDE_DIR})

if(WIN32)
  target_include_directories(
    ${TARGET_NAME}_loadable_extension
    PRIVATE postgres/src/include/port/win32 postgres/src/port
            postgres/src/include/port/win32_msvc)
endif()

target_link_libraries(${TARGET_NAME}_loadable_extension ${OPENSSL_LIBRARIES} mysqlclient z)
set_property(TARGET ${TARGET_NAME}_loadable_extension PROPERTY C_STANDARD 99)

if(WIN32)
  target_link_libraries(${TARGET_NAME}_loadable_extension wsock32 ws2_32
                        wldap32 secur32 crypt32)
endif()

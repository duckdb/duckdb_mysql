include_directories(include)

add_subdirectory(storage)

add_library(
  mysql_ext_library OBJECT
  mysql_connection.cpp
  mysql_extension.cpp
  mysql_scanner.cpp
  mysql_storage.cpp
  mysql_utils.cpp)
set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:mysql_ext_library>
    PARENT_SCOPE)

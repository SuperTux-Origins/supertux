option(USE_SYSTEM_FMT "Use preinstalled fmt if available, must be 8.0.0 or newer" ON)
if(USE_SYSTEM_FMT)
  find_package(fmt 8.0.0 QUIET)
endif()

if(TARGET fmt::fmt)
  message(STATUS "Found fmt")
  add_library(LibFmt ALIAS fmt::fmt)
else()
  set(FMT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/fmt" CACHE PATH
      "Path to fmt sources (CMakeLists.txt)")
  if(EXISTS "${FMT_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "Could NOT find system fmt, using FMT_SOURCE_DIR=${FMT_SOURCE_DIR}")
    add_subdirectory("${FMT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/fmt" EXCLUDE_FROM_ALL)
    add_library(LibFmt INTERFACE IMPORTED)
    set_target_properties(LibFmt PROPERTIES
      INTERFACE_LINK_LIBRARIES "fmt::fmt"
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:fmt::fmt,INTERFACE_INCLUDE_DIRECTORIES>")
  else()
    message(FATAL_ERROR
      "fmt not found (system or FMT_SOURCE_DIR=${FMT_SOURCE_DIR}).\n"
      "  Install libfmt >= 8, or pass -DFMT_SOURCE_DIR=/path/to/fmt")
  endif()
endif()

# EOF #

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  ## Find revision of WC
  mark_as_advanced(GIT_EXECUTABLE)
  find_program(GIT_EXECUTABLE git)
  if(NOT GIT_EXECUTABLE EQUAL "GIT_EXECUTABLE-NOTFOUND")
    include(GetGitRevisionDescription)
    git_describe(VERSION_STRING_GIT "--tags" "--match" "?[0-9]*.[0-9]*.[0-9]*")
    string(REPLACE "v" "" VERSION_LIST ${VERSION_STRING_GIT})
    string(REGEX REPLACE "[^\\-]+\\-([0-9]+)\\-.*" "\\1" VERSION_NUMBER_GIT "${VERSION_LIST}")
    string(REGEX REPLACE "(-|_|\\.)" ";" VERSION_LIST ";${VERSION_LIST}")
  endif()
endif()

get_filename_component(BASEDIR ${CMAKE_SOURCE_DIR} NAME)
if("${VERSION_LIST}" STREQUAL "")
  if(${BASEDIR} MATCHES "supertux-origins-[0-9\\.]*")
    string(REGEX REPLACE "(\\.|_|-)" ";" VERSION_LIST ${BASEDIR})
  endif()
endif()

file(GLOB ORIG_TGZ ../*.orig.tar.gz)
if("${VERSION_LIST}" STREQUAL "" AND (NOT "${ORIG_TGZ}" STREQUAL ""))
  get_filename_component(BASEDIR ${ORIG_TGZ} NAME)
  string(REGEX REPLACE "(\\.|_|-)" ";" VERSION_LIST ${BASEDIR})
endif()

list(LENGTH VERSION_LIST VERSION_LIST_SIZE)

if(${VERSION_LIST_SIZE} GREATER 0)
  list(GET VERSION_LIST 1 MAJOR_VERSION_GIT)
  list(GET VERSION_LIST 2 MINOR_VERSION_GIT)
  list(GET VERSION_LIST 3 PATCH_VERSION_GIT)

  if("${VERSION_STRING_GIT}" STREQUAL "")
    set(VERSION_STRING_GIT "${MAJOR_VERSION_GIT}.${MINOR_VERSION_GIT}.${PATCH_VERSION_GIT}")
  endif()

  configure_file("${CMAKE_SOURCE_DIR}/version.cmake.in" "${CMAKE_SOURCE_DIR}/version.cmake")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/version.cmake")
  # Packaging / cleanSource trees often lack .git and a pre-generated
  # version.cmake. Synthesize one from PROJECT_VERSION_FULL or a default.
  if(DEFINED PROJECT_VERSION_FULL AND NOT PROJECT_VERSION_FULL STREQUAL "")
    set(_ver_full "${PROJECT_VERSION_FULL}")
  else()
    set(_ver_full "0.6.3-dev")
  endif()
  string(REGEX REPLACE "[-+].*" "" _ver_base "${_ver_full}")
  string(REPLACE "." ";" _ver_parts "${_ver_base}")
  list(LENGTH _ver_parts _ver_n)
  set(MAJOR_VERSION_GIT 0)
  set(MINOR_VERSION_GIT 0)
  set(PATCH_VERSION_GIT 0)
  if(_ver_n GREATER 0)
    list(GET _ver_parts 0 MAJOR_VERSION_GIT)
  endif()
  if(_ver_n GREATER 1)
    list(GET _ver_parts 1 MINOR_VERSION_GIT)
  endif()
  if(_ver_n GREATER 2)
    list(GET _ver_parts 2 PATCH_VERSION_GIT)
  endif()
  set(VERSION_STRING_GIT "${_ver_full}")
  set(VERSION_NUMBER_GIT "0")
  set(TWEAK_VERSION_GIT "")
  # Prefer writing into the build dir; also materialize under SOURCE_DIR so
  # the historical include("${CMAKE_SOURCE_DIR}/version.cmake") keeps working
  # in isolated sandboxes (SOURCE_DIR may be read-only — fall back to BINARY).
  set(_ver_out "${CMAKE_SOURCE_DIR}/version.cmake")
  configure_file("${CMAKE_SOURCE_DIR}/version.cmake.in" "${CMAKE_BINARY_DIR}/version.cmake")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/version.cmake" "${_ver_out}"
    ERROR_QUIET RESULT_VARIABLE _ver_copy_rc)
  if(NOT _ver_copy_rc EQUAL 0)
    set(_ver_out "${CMAKE_BINARY_DIR}/version.cmake")
  endif()
  message(STATUS "Generated version.cmake (${_ver_full}) at ${_ver_out}")
  include("${_ver_out}")
else()
  include("${CMAKE_SOURCE_DIR}/version.cmake")
endif()

if(FORCE_VERSION_STRING)
  set(SUPERTUX_VERSION_STRING "${FORCE_VERSION_STRING}")
endif()
set(SUPERTUX_VERSION ${SUPERTUX_VERSION_STRING})

configure_file(version.h.in ${CMAKE_BINARY_DIR}/version.h )

set_source_files_properties(${CMAKE_BINARY_DIR}/version.h
  PROPERTIES GENERATED true)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/supertux/main.cpp
  PROPERTIES OBJECT_DEPENDS "${CMAKE_BINARY_DIR}/version.h")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/supertux/title_screen.cpp
  PROPERTIES OBJECT_DEPENDS "${CMAKE_BINARY_DIR}/version.h")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/addon/addon_manager.cpp
  PROPERTIES OBJECT_DEPENDS "${CMAKE_BINARY_DIR}/version.h")

# EOF #

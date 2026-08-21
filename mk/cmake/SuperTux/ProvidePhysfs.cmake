# PhysFS — system package preferred when it has PHYSFS_getPrefDir; otherwise
# build from external/physfs or PHYSFS_SOURCE_DIR (for cross / wasm / NDK).

find_package(PhysFS QUIET)

# Detect PHYSFS_getPrefDir without relying solely on try_compile.
# Under MinGW cross-compile, check_symbol_exists often fails even when the
# prebuilt physfs-win32 (3.0.2+) header declares the symbol — CMAKE then
# falls through to ExternalProject and dies because external/physfs is not
# checked out.
set(HAVE_PHYSFS_GETPREFDIR FALSE)
if(PHYSFS_LIBRARY)
  set(_physfs_hdr "")
  if(PHYSFS_INCLUDE_DIR AND EXISTS "${PHYSFS_INCLUDE_DIR}/physfs.h")
    set(_physfs_hdr "${PHYSFS_INCLUDE_DIR}/physfs.h")
  elseif(DEFINED PhysFS_INCLUDE_DIR AND EXISTS "${PhysFS_INCLUDE_DIR}/physfs.h")
    set(_physfs_hdr "${PhysFS_INCLUDE_DIR}/physfs.h")
    set(PHYSFS_INCLUDE_DIR "${PhysFS_INCLUDE_DIR}")
  endif()
  # Also accept imported target include dirs (some FindPhysFS / Config packages).
  if(NOT _physfs_hdr AND TARGET PhysFS::PhysFS)
    get_target_property(_physfs_incs PhysFS::PhysFS INTERFACE_INCLUDE_DIRECTORIES)
    if(_physfs_incs)
      foreach(_inc ${_physfs_incs})
        if(EXISTS "${_inc}/physfs.h")
          set(_physfs_hdr "${_inc}/physfs.h")
          set(PHYSFS_INCLUDE_DIR "${_inc}")
          break()
        endif()
      endforeach()
    endif()
  endif()

  if(_physfs_hdr)
    file(STRINGS "${_physfs_hdr}" _physfs_prefdir_lines REGEX "PHYSFS_getPrefDir")
    if(_physfs_prefdir_lines)
      set(HAVE_PHYSFS_GETPREFDIR TRUE)
      message(STATUS "PHYSFS_getPrefDir: declared in ${_physfs_hdr}")
    endif()
  endif()

  if(NOT HAVE_PHYSFS_GETPREFDIR)
    set(CMAKE_REQUIRED_LIBRARIES ${PHYSFS_LIBRARY})
    if(PHYSFS_INCLUDE_DIR)
      set(CMAKE_REQUIRED_INCLUDES ${PHYSFS_INCLUDE_DIR})
    endif()
    check_symbol_exists("PHYSFS_getPrefDir" "physfs.h" HAVE_PHYSFS_GETPREFDIR)
  endif()
endif()

if(HAVE_PHYSFS_GETPREFDIR AND NOT EMSCRIPTEN AND NOT ANDROID)
  set(USE_SYSTEM_PHYSFS ON CACHE BOOL "Use preinstalled physfs (must support getPrefDir)")
else()
  set(USE_SYSTEM_PHYSFS OFF CACHE BOOL "Use preinstalled physfs (must support getPrefDir)")
endif()

if(USE_SYSTEM_PHYSFS)
  message(STATUS "Using system PhysFS: ${PHYSFS_LIBRARY}")
  add_library(LibPhysfs INTERFACE)
  set_target_properties(LibPhysfs PROPERTIES
    INTERFACE_LINK_LIBRARIES "${PHYSFS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSFS_INCLUDE_DIR}")
else()
  # Source tree: submodule, or override for cross builds (wasm/Android/R36S/MinGW).
  set(PHYSFS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/physfs" CACHE PATH
      "Path to physfs sources (CMakeLists.txt)")
  if(NOT EXISTS "${PHYSFS_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
      "physfs sources not found at PHYSFS_SOURCE_DIR=${PHYSFS_SOURCE_DIR}.\n"
      "  Checkout external/physfs or pass -DPHYSFS_SOURCE_DIR=/path/to/physfs\n"
      "  (needed when system PhysFS is unavailable or lacks PHYSFS_getPrefDir).\n"
      "  PHYSFS_LIBRARY=${PHYSFS_LIBRARY}\n"
      "  PHYSFS_INCLUDE_DIR=${PHYSFS_INCLUDE_DIR}\n"
      "  HAVE_PHYSFS_GETPREFDIR=${HAVE_PHYSFS_GETPREFDIR}")
  endif()

  if(WIN32 AND NOT EMSCRIPTEN)
    set(PHYSFS_BUILD_SHARED TRUE)
    set(PHYSFS_BUILD_STATIC FALSE)
  else()
    set(PHYSFS_BUILD_SHARED FALSE)
    set(PHYSFS_BUILD_STATIC TRUE)
  endif()

  set(PHYSFS_PREFIX ${CMAKE_BINARY_DIR}/physfs)
  # Always install into lib/ (not lib64) so IMPORTED_LOCATION / byproducts match.
  set(PHYSFS_LIBDIR "${PHYSFS_PREFIX}/lib")
  include(ExternalProject)
  ExternalProject_Add(physfs_project
    SOURCE_DIR "${PHYSFS_SOURCE_DIR}"
    BUILD_BYPRODUCTS
    "${PHYSFS_PREFIX}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}physfs${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "${PHYSFS_LIBDIR}/physfs${CMAKE_LINK_LIBRARY_SUFFIX}"
    "${PHYSFS_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}physfs${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CMAKE_ARGS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX=${PHYSFS_PREFIX}
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DLIB_SUFFIX=
    -DPHYSFS_BUILD_SHARED=${PHYSFS_BUILD_SHARED}
    -DPHYSFS_BUILD_STATIC=${PHYSFS_BUILD_STATIC}
    -DPHYSFS_BUILD_TEST=FALSE
    -DPHYSFS_BUILD_DOCS=FALSE
    # Do not forward parent USE_FLAGS (emscripten ports) into physfs.
    -DCMAKE_C_FLAGS=
    -DCMAKE_CXX_FLAGS=)

  file(MAKE_DIRECTORY "${PHYSFS_PREFIX}/include/")

  if(WIN32 AND NOT EMSCRIPTEN)
    add_library(LibPhysfs SHARED IMPORTED)
    set_target_properties(LibPhysfs PROPERTIES
      IMPORTED_LOCATION "${PHYSFS_PREFIX}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}physfs${CMAKE_SHARED_LIBRARY_SUFFIX}"
      IMPORTED_IMPLIB "${PHYSFS_LIBDIR}/physfs${CMAKE_LINK_LIBRARY_SUFFIX}"
      INTERFACE_INCLUDE_DIRECTORIES "${PHYSFS_PREFIX}/include/")
  else()
    add_library(LibPhysfs STATIC IMPORTED)
    set_target_properties(LibPhysfs PROPERTIES
      IMPORTED_LOCATION "${PHYSFS_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}physfs${CMAKE_STATIC_LIBRARY_SUFFIX}"
      INTERFACE_INCLUDE_DIRECTORIES "${PHYSFS_PREFIX}/include/")
  endif()

  if(APPLE)
    set_target_properties(LibPhysfs PROPERTIES
      INTERFACE_LINK_LIBRARIES "-framework CoreFoundation;-framework Foundation;-framework IOKit")
  endif()

  add_dependencies(LibPhysfs physfs_project)

  if(WIN32 AND NOT EMSCRIPTEN)
    get_property(PHYSFS_LIB_PATH TARGET LibPhysfs PROPERTY IMPORTED_LOCATION)
    install(FILES ${PHYSFS_LIB_PATH} DESTINATION "${INSTALL_SUBDIR_BIN}")
  endif()
endif()

mark_as_advanced(
  PHYSFS_INCLUDE_DIR
  PHYSFS_LIBRARY
  PHYSFS_SOURCE_DIR
  )

# EOF #

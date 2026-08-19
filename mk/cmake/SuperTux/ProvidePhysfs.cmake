# PhysFS — system package preferred when it has PHYSFS_getPrefDir; otherwise
# build from external/physfs or PHYSFS_SOURCE_DIR (for cross / wasm / NDK).

find_package(PhysFS QUIET)

if(PHYSFS_LIBRARY)
  set(CMAKE_REQUIRED_LIBRARIES ${PHYSFS_LIBRARY})
  check_symbol_exists("PHYSFS_getPrefDir" "${PHYSFS_INCLUDE_DIR}/physfs.h" HAVE_PHYSFS_GETPREFDIR)
endif()

if(HAVE_PHYSFS_GETPREFDIR AND NOT EMSCRIPTEN AND NOT ANDROID)
  set(USE_SYSTEM_PHYSFS ON CACHE BOOL "Use preinstalled physfs (must support getPrefDir)")
else()
  set(USE_SYSTEM_PHYSFS OFF CACHE BOOL "Use preinstalled physfs (must support getPrefDir)")
endif()

if(USE_SYSTEM_PHYSFS)
  add_library(LibPhysfs INTERFACE)
  set_target_properties(LibPhysfs PROPERTIES
    INTERFACE_LINK_LIBRARIES "${PHYSFS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSFS_INCLUDE_DIR}")
else()
  # Source tree: submodule, or override for cross builds (wasm/Android/R36S).
  set(PHYSFS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/physfs" CACHE PATH
      "Path to physfs sources (CMakeLists.txt)")
  if(NOT EXISTS "${PHYSFS_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
      "physfs sources not found at PHYSFS_SOURCE_DIR=${PHYSFS_SOURCE_DIR}.\n"
      "  Checkout external/physfs or pass -DPHYSFS_SOURCE_DIR=/path/to/physfs\n"
      "  (needed for EMSCRIPTEN/Android when system PhysFS is unavailable).")
  endif()

  if(WIN32 AND NOT EMSCRIPTEN)
    set(PHYSFS_BUILD_SHARED TRUE)
    set(PHYSFS_BUILD_STATIC FALSE)
  else()
    set(PHYSFS_BUILD_SHARED FALSE)
    set(PHYSFS_BUILD_STATIC TRUE)
  endif()

  set(PHYSFS_PREFIX ${CMAKE_BINARY_DIR}/physfs)
  include(ExternalProject)
  ExternalProject_Add(physfs_project
    SOURCE_DIR "${PHYSFS_SOURCE_DIR}"
    BUILD_BYPRODUCTS
    "${PHYSFS_PREFIX}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}physfs${CMAKE_SHARED_LIBRARY_SUFFIX}"
    "${PHYSFS_PREFIX}/lib${LIB_SUFFIX}/physfs${CMAKE_LINK_LIBRARY_SUFFIX}"
    "${PHYSFS_PREFIX}/lib${LIB_SUFFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}physfs${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CMAKE_ARGS
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    -DCMAKE_INSTALL_PREFIX=${PHYSFS_PREFIX}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DLIB_SUFFIX=${LIB_SUFFIX}
    -DPHYSFS_BUILD_SHARED=${PHYSFS_BUILD_SHARED}
    -DPHYSFS_BUILD_STATIC=${PHYSFS_BUILD_STATIC}
    -DPHYSFS_BUILD_TEST=FALSE
    -DPHYSFS_BUILD_DOCS=FALSE)

  file(MAKE_DIRECTORY "${PHYSFS_PREFIX}/include/")

  if(WIN32 AND NOT EMSCRIPTEN)
    add_library(LibPhysfs SHARED IMPORTED)
    set_target_properties(LibPhysfs PROPERTIES
      IMPORTED_LOCATION "${PHYSFS_PREFIX}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}physfs${CMAKE_SHARED_LIBRARY_SUFFIX}"
      IMPORTED_IMPLIB "${PHYSFS_PREFIX}/lib${LIB_SUFFIX}/physfs${CMAKE_LINK_LIBRARY_SUFFIX}"
      INTERFACE_INCLUDE_DIRECTORIES "${PHYSFS_PREFIX}/include/")
  else()
    add_library(LibPhysfs STATIC IMPORTED)
    set_target_properties(LibPhysfs PROPERTIES
      IMPORTED_LOCATION "${PHYSFS_PREFIX}/lib${LIB_SUFFIX}/${CMAKE_STATIC_LIBRARY_PREFIX}physfs${CMAKE_STATIC_LIBRARY_SUFFIX}"
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

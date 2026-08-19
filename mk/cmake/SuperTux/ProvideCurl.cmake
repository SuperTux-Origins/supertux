## curl is optional on constrained targets (Emscripten, R36S sysroot without
## libcurl headers). Desktop / Windows keep REQUIRED.

if(EMSCRIPTEN OR SUPERTUX_R36S)
  message(STATUS "Skipping libcurl (EMSCRIPTEN or SUPERTUX_R36S)")
  set(HAVE_LIBCURL FALSE)
  # Empty interface so any residual LibCurl link lines stay valid if guarded.
  if(NOT TARGET LibCurl)
    add_library(LibCurl INTERFACE IMPORTED)
  endif()
else()
  find_package(CURL REQUIRED)

  add_library(LibCurl INTERFACE IMPORTED)
  set_target_properties(LibCurl PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}")
  if(WIN32)
    target_link_libraries(LibCurl INTERFACE ${CURL_LIBRARY})
  else()
    set_target_properties(LibCurl PROPERTIES
      INTERFACE_LINK_LIBRARIES "${CURL_LIBRARY}")
  endif()

  set(HAVE_LIBCURL TRUE)
endif()

# EOF #

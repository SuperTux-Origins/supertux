option(ENABLE_OPENGL "Enable OpenGL support" ON)
option(ENABLE_OPENGLES2 "Enable OpenGLES2 support" OFF)

if(ENABLE_OPENGL)
  if(ENABLE_OPENGLES2)
    message(STATUS "Checking for OpenGLES2")

    set(HAVE_OPENGL TRUE)
    list(APPEND OPENGL_COMPILE_DEFINITIONS "USE_OPENGLES2")

    # Emscripten: WebGL via FULL_ES2.  Android: GLES comes with SDL / NDK.
    # Desktop GLES2 (or R36S with sysroot pkg-config): link glesv2 explicitly.
    if(NOT EMSCRIPTEN AND NOT ANDROID AND NOT DEFINED ANDROID)
      pkg_check_modules(GLESV2 REQUIRED glesv2)
      list(APPEND OPENGL_INCLUDE_DIRECTORIES "${GLESV2_INCLUDE_DIRS}")
      list(APPEND OPENGL_LINK_LIBRARIES "${GLESV2_LIBRARIES}")
    endif()
  else()
    message(STATUS "Checking for OpenGL (GLAD loader)")

    set(HAVE_OPENGL TRUE)

    # Vendored GLAD (OpenGL 3.3 core). Loads entry points via SDL_GL_GetProcAddress
    set(GLAD_DIR "${CMAKE_SOURCE_DIR}/external/glad")
    if(NOT EXISTS "${GLAD_DIR}/src/gl.c")
      message(FATAL_ERROR "GLAD sources missing at ${GLAD_DIR} (expected src/gl.c)")
    endif()

    add_library(glad STATIC "${GLAD_DIR}/src/gl.c")
    target_include_directories(glad PUBLIC "${GLAD_DIR}/include")
    # glad is pure C; silence noisy warnings from generated code if any
    set_target_properties(glad PROPERTIES C_STANDARD 99)

    list(APPEND OPENGL_LINK_LIBRARIES glad)
    list(APPEND OPENGL_INCLUDE_DIRECTORIES "${GLAD_DIR}/include")

    # Platform GL library (symbols / ICD). Prefer CMake's OpenGL::GL
    # when present Linking OpenGL::GL may still pull GLX transitively
    # on some systems; the game talks to GL only through GLAD + SDL
    # context.
    if(WIN32)
      list(APPEND OPENGL_LINK_LIBRARIES opengl32)
    else()
      set(OpenGL_GL_PREFERENCE "GLVND")
      find_package(OpenGL)
      if(TARGET OpenGL::GL)
        list(APPEND OPENGL_LINK_LIBRARIES OpenGL::GL)
      elseif(OPENGL_opengl_LIBRARY)
        list(APPEND OPENGL_LINK_LIBRARIES ${OPENGL_opengl_LIBRARY})
      elseif(OPENGL_gl_LIBRARY)
        list(APPEND OPENGL_LINK_LIBRARIES ${OPENGL_gl_LIBRARY})
      endif()
    endif()
  endif()

  if(NOT HAVE_OPENGL)
    message(STATUS "  OpenGL not found")
  else()
    add_library(LibOpenGL INTERFACE)
    set_target_properties(LibOpenGL PROPERTIES
      INTERFACE_LINK_LIBRARIES "${OPENGL_LINK_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIRECTORIES}"
      INTERFACE_COMPILE_DEFINITIONS "${OPENGL_COMPILE_DEFINITIONS}"
      )

    message(STATUS "  OPENGL_LINK_LIBRARIES: ${OPENGL_LINK_LIBRARIES}")
    message(STATUS "  OPENGL_INCLUDE_DIRECTORIES: ${OPENGL_INCLUDE_DIRECTORIES}")
    message(STATUS "  OPENGL_COMPILE_DEFINITIONS: ${OPENGL_COMPILE_DEFINITIONS}")
  endif()
endif()

# EOF #

# glm is header-only. Cross builds (R36S / ArkOS) often have no glm in the
# sysroot and CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY hides the host package —
# allow an explicit include dir from packaging (same idea as Pingus
# PINGUS_GLM_INCLUDE_DIR).

if(SUPERTUX_GLM_INCLUDE_DIR)
  message(STATUS "Using glm headers: ${SUPERTUX_GLM_INCLUDE_DIR}")
  if(NOT TARGET glm::glm)
    add_library(glm::glm INTERFACE IMPORTED)
    set_target_properties(glm::glm PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SUPERTUX_GLM_INCLUDE_DIR}")
  endif()
  add_library(LibGlm INTERFACE IMPORTED)
  set_target_properties(LibGlm PROPERTIES
    INTERFACE_LINK_LIBRARIES "glm::glm"
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${SUPERTUX_GLM_INCLUDE_DIR}"
    INTERFACE_COMPILE_DEFINITIONS "GLM_ENABLE_EXPERIMENTAL")
else()
  find_package(glm QUIET)

  if(glm_FOUND)
    if(TARGET glm::glm)
      message(STATUS "Found glm")

      add_library(LibGlm INTERFACE IMPORTED)
      set_target_properties(LibGlm PROPERTIES
        INTERFACE_LINK_LIBRARIES "glm::glm"
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:glm::glm,INTERFACE_INCLUDE_DIRECTORIES>"
        INTERFACE_COMPILE_DEFINITIONS "GLM_ENABLE_EXPERIMENTAL")
    else()
      # fallback for old glm version in EmuELEC and Nix
      message(STATUS "Found glm: ${GLM_INCLUDE_DIR}")

      add_library(LibGlm INTERFACE IMPORTED)
      set_target_properties(LibGlm PROPERTIES
        INTERFACE_LINK_LIBRARIES "glm"
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:glm,INTERFACE_INCLUDE_DIRECTORIES>"
        INTERFACE_COMPILE_DEFINITIONS "GLM_ENABLE_EXPERIMENTAL")
    endif()
  else()
    # fallback for old glm version in UBPorts / restricted FIND_ROOT
    find_path(GLM_INCLUDE_DIR
      NAMES glm/glm.hpp
      PATHS ${GLM_ROOT_DIR}/include
      NO_CMAKE_FIND_ROOT_PATH)
    if(NOT GLM_INCLUDE_DIR)
      message(FATAL_ERROR "glm library missing")
    endif()

    message(STATUS "Found glm: ${GLM_INCLUDE_DIR}")
    add_library(LibGlm INTERFACE IMPORTED)
    set_target_properties(LibGlm PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}"
      INTERFACE_COMPILE_DEFINITIONS "GLM_ENABLE_EXPERIMENTAL")
  endif()
endif()

# EOF #

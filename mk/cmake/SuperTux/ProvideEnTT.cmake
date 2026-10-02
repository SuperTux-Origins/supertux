# EnTT is header-only and vendored in external/entt so that all targets
# (Linux, MinGW, wasm, Android, R36S) build against the same version.

add_library(LibEnTT INTERFACE IMPORTED)
set_target_properties(LibEnTT PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/entt/include")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/external/entt/VERSION" ENTT_VENDORED_VERSION)
string(STRIP "${ENTT_VENDORED_VERSION}" ENTT_VENDORED_VERSION)
message(STATUS "Using vendored EnTT ${ENTT_VENDORED_VERSION}")

# EOF #

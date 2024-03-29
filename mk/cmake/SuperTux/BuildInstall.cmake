if(WIN32 AND NOT UNIX)
  if(VCPKG_BUILD)
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/" DESTINATION ${INSTALL_SUBDIR_BIN} FILES_MATCHING PATTERN "*.dll")
  else()
    install(FILES ${DLLS} DESTINATION ${INSTALL_SUBDIR_BIN})
  endif()

  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.png ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.ico DESTINATION ".")

  option(PACKAGE_VCREDIST "Package the VCREDIST libraries with the program" OFF)

  if(PACKAGE_VCREDIST)
    set(CMAKE_INSTALL_UCRT_LIBRARIES true)
    include(InstallRequiredSystemLibraries)
  endif()

else()
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND DISABLE_CPACK_BUNDLING)

    set(INFOPLIST_CFBUNDLEEXECUTABLE "supertux-origins")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/darwin/info.plist.in ${CMAKE_BINARY_DIR}/tools/darwin/info.plist)
    install(FILES ${CMAKE_BINARY_DIR}/tools/darwin/info.plist DESTINATION "SuperTux.app/Contents/")
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tools/darwin/receipt DESTINATION "SuperTux.app/Contents/_MASReceipt/")

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.png ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.icns DESTINATION "SuperTux.app/Contents/Resources/")

  else()

    if(UBUNTU_TOUCH)
      set(LINUX_DESKTOP_ICON "assets/supertux-256x256.png")
      # FIXME: The "install" folder is a folder managed by Clickable and shouldn't be hardcoded here
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/supertux-origins.desktop.in "install/supertux-origins.desktop")
      install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/mk/clickable/supertux-origins.apparmor DESTINATION ".")
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mk/clickable/manifest.json.in ${CMAKE_CURRENT_BINARY_DIR}/install/manifest.json)
      set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/supertux-origins")
      install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux-256x256.png DESTINATION "assets")
    else()
      set(LINUX_DESKTOP_ICON "supertux-origins")
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/supertux-origins.desktop.in "supertux-origins.desktop")
      install(FILES ${CMAKE_BINARY_DIR}/supertux-origins.desktop DESTINATION "share/applications")
      set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${INSTALL_SUBDIR_BIN}/supertux-origins")
      install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.png ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.xpm DESTINATION "share/pixmaps/")
    endif()

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/supertux-origins.svg DESTINATION "share/icons/hicolor/scalable/apps")

  endif()
endif()

if(UBUNTU_TOUCH)
  install(TARGETS supertux-origins DESTINATION ".")
else()
  install(TARGETS supertux-origins DESTINATION ${INSTALL_SUBDIR_BIN})
endif()

if(EMSCRIPTEN)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mk/emscripten/template.html.in ${CMAKE_CURRENT_BINARY_DIR}/template.html)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mk/emscripten/supertux-origins.png ${CMAKE_CURRENT_BINARY_DIR}/supertux-origins.png COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mk/emscripten/supertux-origins_bkg.png ${CMAKE_CURRENT_BINARY_DIR}/supertux-origins_bkg.png COPYONLY)
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt ${CMAKE_CURRENT_SOURCE_DIR}/NEWS.md DESTINATION ${INSTALL_SUBDIR_DOC})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/credits.stxt DESTINATION ${INSTALL_SUBDIR_SHARE})

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/data/images
  ${CMAKE_CURRENT_SOURCE_DIR}/data/fonts
  ${CMAKE_CURRENT_SOURCE_DIR}/data/music
  ${CMAKE_CURRENT_SOURCE_DIR}/data/particles
  ${CMAKE_CURRENT_SOURCE_DIR}/data/scripts
  ${CMAKE_CURRENT_SOURCE_DIR}/data/shader
  ${CMAKE_CURRENT_SOURCE_DIR}/data/speech
  ${CMAKE_CURRENT_SOURCE_DIR}/data/sounds
  DESTINATION ${INSTALL_SUBDIR_SHARE})

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/levels
    DESTINATION ${INSTALL_SUBDIR_SHARE}
    PATTERN "data/levels/test" EXCLUDE
    PATTERN "data/levels/test_old" EXCLUDE
    PATTERN "data/levels/incubator" EXCLUDE
    )
else()
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/levels
    DESTINATION ${INSTALL_SUBDIR_SHARE}
    PATTERN "data/levels/misc/menu.stl.in" EXCLUDE)
endif()

# move some config clutter to the advanced section
mark_as_advanced(
  INSTALL_SUBDIR_BIN
  INSTALL_SUBDIR_SHARE
  INSTALL_SUBDIR_DOC
  )

# EOF #

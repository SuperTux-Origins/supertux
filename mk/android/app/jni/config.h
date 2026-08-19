#ifndef CONFIG_H
#define CONFIG_H

#define PACKAGE_NAME "supertux-origins"
#define INSTALL_SUBDIR_BIN "bin"
#define INSTALL_SUBDIR_SHARE "data"
#define BUILD_DATA_DIR "."
#define BUILD_CONFIG_DATA_DIR "."

/* GLES2 path — no desktop GL */
/* #undef HAVE_OPENGL */
/* #undef HAVE_LIBCURL */
/* #undef ENABLE_SQDBG */
/* #undef ENABLE_DISCORD */
/* #undef STEAM_BUILD */

#define UBUNTU_TOUCH
#define HIDE_NONMOBILE_OPTIONS
#define REMOVE_QUIT_BUTTON

#endif /* CONFIG_H */

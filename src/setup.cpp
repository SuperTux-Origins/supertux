// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include "platform_config.h"
#include <SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#ifdef __ANDROID__
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#endif
#ifndef NOOPENGL
#include "gl_compat.h"
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#ifndef WIN32
#include <libgen.h>
#endif
#include <ctype.h>

#include "defines.h"
#include "globals.h"
#include "platform.h"
#include "setup.h"
#include "game_file.h"
#include "touch_controls.h"
#include "screen.h"
#include "texture.h"
#include "menu.h"
#include "gameloop.h"
#include "configfile.h"
#include "scene.h"
#include "worldmap.h"
#include "resources.h"
#include "intro.h"
#ifndef NOSOUND
#include "music_manager.h"
#endif

#include "player.h"

#ifdef WIN32
#define mkdir(dir, mode)    mkdir(dir)
// on win32 we typically don't want LFS paths
#undef DATA_PREFIX
#define DATA_PREFIX "./data/"
#endif

/* Screen properties: */
/* Don't use this to test for the actual screen sizes. Use screen->w/h instead! */
#ifndef RES320X240
#define SCREEN_W 640
#define SCREEN_H 480
#else
#define SCREEN_W 320
#define SCREEN_H 240
#endif

#ifdef GP2X
#define DATA_PREFIX "data/"
#endif

/* Local function prototypes: */

void seticon(void);
void usage(char * prog, int ret);

/* Does the given file exist and is it accessible? */
int faccessible(const char *filename)
{
  if (!filename || !filename[0])
    return false;
  /* Real filesystem (mods, installs) or APK assets via open_game_file. */
  if (game_file_exists(filename))
    return true;
  struct stat filestat;
  if (stat(filename, &filestat) == -1)
    return false;
  return S_ISREG(filestat.st_mode) ? true : false;
}

/* Can we write to this location? */
int fwriteable(const char *filename)
{
  FILE* fi;
  fi = fopen(filename, "wa");
  if (fi == NULL)
    {
      return false;
    }
  fclose(fi);
  return true;
}

/* Makes sure a directory is created in either the SuperTux home directory or the SuperTux base directory.*/
int fcreatedir(const char* relative_dir)
{
  char path[1024];
  snprintf(path, 1024, "%s/%s/", st_dir, relative_dir);
  if(mkdir(path,0755) != 0)
    {
      snprintf(path, 1024, "%s/%s/", datadir.c_str(), relative_dir);
      if(mkdir(path,0755) != 0)
        {
          return false;
        }
      else
        {
          return true;
        }
    }
  else
    {
      return true;
    }
}

FILE * opendata(const char * rel_filename, const char * mode)
{
  char * filename = NULL;
  FILE * fi;

  filename = (char *) malloc(sizeof(char) * (strlen(st_dir) +
                                             strlen(rel_filename) + 1));

  strcpy(filename, st_dir);
  /* Open the high score file: */

  strcat(filename, rel_filename);

  /* Try opening the file: */
  fi = fopen(filename, mode);

  if (fi == NULL)
    {
      fprintf(stderr, "Warning: Unable to open the file \"%s\" ", filename);

      if (strcmp(mode, "r") == 0)
        fprintf(stderr, "for read!!!\n");
      else if (strcmp(mode, "w") == 0)
        fprintf(stderr, "for write!!!\n");
    }
  free( filename );

  return(fi);
}

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Join path components into dest; returns false if the result would truncate. */
static bool
path_join2(char* dest, size_t dest_sz, const char* a, const char* b)
{
  int n = snprintf(dest, dest_sz, "%s/%s", a, b);
  return n >= 0 && (size_t)n < dest_sz;
}

static bool
path_join3(char* dest, size_t dest_sz, const char* a, const char* b, const char* c)
{
  int n = snprintf(dest, dest_sz, "%s/%s/%s", a, b, c);
  return n >= 0 && (size_t)n < dest_sz;
}

static bool
path_join4(char* dest, size_t dest_sz,
           const char* a, const char* b, const char* c, const char* d)
{
  int n = snprintf(dest, dest_sz, "%s/%s/%s/%s", a, b, c, d);
  return n >= 0 && (size_t)n < dest_sz;
}

#ifdef __ANDROID__
static void android_list_asset_dir(const char* asset_dir, string_list_type* out);

/* AssetManager paths are relative to assets/ with no leading slash. */
static void
android_asset_dir_normalize(const char* rel_path, char* out, size_t out_sz)
{
  const char* p = rel_path ? rel_path : "";
  while (*p == '/' || *p == '\\')
    ++p;
  snprintf(out, out_sz, "%s", p);
  size_t n = strlen(out);
  while (n > 0 && (out[n - 1] == '/' || out[n - 1] == '\\'))
    out[--n] = '\0';
}

/* True if name is already in the string_list (must be before dfiles/dsubdirs). */
static int
string_list_contains(const string_list_type* list, const char* name)
{
  if (!list || !name)
    return 0;
  for (int i = 0; i < list->num_items; ++i)
    if (strcmp(list->item[i], name) == 0)
      return 1;
  return 0;
}
#endif

/* Get all names of sub-directories in a certain directory. */
/* Returns the number of sub-directories found. */
/* Note: The user has to free the allocated space. */
string_list_type dsubdirs(const char *rel_path,const  char* expected_file)
{
  DIR *dirStructP;
  struct dirent *direntp;
  string_list_type sdirs;
  char filename[PATH_MAX];
  char path[PATH_MAX];

  string_list_init(&sdirs);
  if (!path_join2(path, sizeof(path), st_dir, rel_path))
    return sdirs;
  if((dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[PATH_MAX];
          struct stat buf;

          if (!path_join2(absolute_filename, sizeof(absolute_filename),
                          path, direntp->d_name))
            continue;

          if (stat(absolute_filename, &buf) == 0 && S_ISDIR(buf.st_mode))
            {
              if(expected_file != NULL)
                {
                  if (!path_join3(filename, sizeof(filename),
                                  path, direntp->d_name, expected_file))
                    continue;
                  if(!faccessible(filename))
                    continue;
                }

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

  if (!path_join2(path, sizeof(path), datadir.c_str(), rel_path))
    return sdirs;
  if((dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[PATH_MAX];
          struct stat buf;

          if (!path_join2(absolute_filename, sizeof(absolute_filename),
                          path, direntp->d_name))
            continue;

          if (stat(absolute_filename, &buf) == 0 && S_ISDIR(buf.st_mode))
            {
              if(expected_file != NULL)
                {
                  if (!path_join3(filename, sizeof(filename),
                                  path, direntp->d_name, expected_file))
                    continue;
                  if(!faccessible(filename))
                    {
                      continue;
                    }
                  else
                    {
                      if (!path_join4(filename, sizeof(filename),
                                      st_dir, rel_path, direntp->d_name, expected_file))
                        continue;
                      if(faccessible(filename))
                        continue;
                    }
                }

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

#ifdef __ANDROID__
  /* APK assets are not a real directory tree — list via AssetManager. */
  {
    string_list_type asset_names;
    string_list_init(&asset_names);
    char asset_dir[PATH_MAX];
    android_asset_dir_normalize(rel_path, asset_dir, sizeof(asset_dir));
    android_list_asset_dir(asset_dir, &asset_names);
    st_vlog("[data] dsubdirs assets '%s' → %d name(s)\n",
            asset_dir, asset_names.num_items);
    for (int i = 0; i < asset_names.num_items; ++i)
      {
        char child_path[PATH_MAX];
        char probe[PATH_MAX];
        if (!path_join2(child_path, sizeof(child_path), asset_dir, asset_names.item[i]))
          continue;
        if (expected_file != NULL)
          {
            if (!path_join2(probe, sizeof(probe), child_path, expected_file))
              continue;
            /* game path uses datadir prefix for open_game_file stripping */
            char full[PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", datadir.c_str(), probe);
            if (!faccessible(full) && !faccessible(probe))
              continue;
          }
        /* Skip if already listed from filesystem. */
        int dup = 0;
        for (int j = 0; j < sdirs.num_items; ++j)
          if (strcmp(sdirs.item[j], asset_names.item[i]) == 0)
            { dup = 1; break; }
        if (!dup)
          string_list_add_item(&sdirs, asset_names.item[i]);
      }
    string_list_free(&asset_names);
  }
#endif

  return sdirs;
}

string_list_type dfiles(const char *rel_path, const  char* glob, const  char* exception_str)
{
  DIR *dirStructP;
  struct dirent *direntp;
  string_list_type sdirs;
  char path[PATH_MAX];

  string_list_init(&sdirs);
  if (path_join2(path, sizeof(path), st_dir, rel_path)
      && (dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[PATH_MAX];
          struct stat buf;

          if (!path_join2(absolute_filename, sizeof(absolute_filename),
                          path, direntp->d_name))
            continue;

          if (stat(absolute_filename, &buf) == 0 && S_ISREG(buf.st_mode))
            {
              if(exception_str != NULL)
                {
                  if(strstr(direntp->d_name,exception_str) != NULL)
                    continue;
                }
              if(glob != NULL)
                if(strstr(direntp->d_name,glob) == NULL)
                  continue;

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

  if (path_join2(path, sizeof(path), datadir.c_str(), rel_path)
      && (dirStructP = opendir(path)) != NULL)
    {
      while((direntp = readdir(dirStructP)) != NULL)
        {
          char absolute_filename[PATH_MAX];
          struct stat buf;

          if (!path_join2(absolute_filename, sizeof(absolute_filename),
                          path, direntp->d_name))
            continue;

          if (stat(absolute_filename, &buf) == 0 && S_ISREG(buf.st_mode))
            {
              if(exception_str != NULL)
                {
                  if(strstr(direntp->d_name,exception_str) != NULL)
                    continue;
                }
              if(glob != NULL)
                if(strstr(direntp->d_name,glob) == NULL)
                  continue;

              string_list_add_item(&sdirs,direntp->d_name);
            }
        }
      closedir(dirStructP);
    }

#ifdef __ANDROID__
  /* APK assets: always run (opendir cannot see assets/). */
  {
    string_list_type asset_names;
    string_list_init(&asset_names);
    char asset_dir[PATH_MAX];
    android_asset_dir_normalize(rel_path, asset_dir, sizeof(asset_dir));
    android_list_asset_dir(asset_dir, &asset_names);
    st_vlog("[data] dfiles assets '%s' → %d name(s) (glob=%s)\n",
            asset_dir, asset_names.num_items, glob ? glob : "(any)");
    for (int i = 0; i < asset_names.num_items; ++i)
      {
        const char* name = asset_names.item[i];
        if (exception_str != NULL && strstr(name, exception_str) != NULL)
          continue;
        if (glob != NULL && strstr(name, glob) == NULL)
          continue;
        if (!string_list_contains(&sdirs, name))
          string_list_add_item(&sdirs, name);
      }
    string_list_free(&asset_names);
  }
#endif

  return sdirs;
}

void free_strings(char **strings, int num)
{
  int i;
  for(i=0; i < num; ++i)
    free(strings[i]);
}

/* --- SETUP --- */
/* Set SuperTux configuration and save directories */

static std::string userdir_override;

#ifdef __ANDROID__
/*
 * List names under an APK assets/ subdirectory.
 *
 * Prefer NDK AAssetDir (files only) — works for assets injected via zip into
 * the APK. Supplement with Java AssetManager.list() for subdirectory names
 * (needed by dsubdirs). Zip-injected trees often make list() return empty
 * while SDL_RWFromFile / AAsset still open files.
 */
static void
android_list_asset_dir(const char* asset_dir, string_list_type* out)
{
  char norm[PATH_MAX];
  android_asset_dir_normalize(asset_dir, norm, sizeof(norm));

  JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
  jobject activity = (jobject)SDL_AndroidGetActivity();
  if (!env || !activity)
    {
      st_vlog("[data] android_list_asset_dir('%s'): no JNI env/activity\n", norm);
      return;
    }

  jclass act_class = env->GetObjectClass(activity);
  jmethodID get_assets = env->GetMethodID(
      act_class, "getAssets", "()Landroid/content/res/AssetManager;");
  if (!get_assets)
    {
      st_vlog("[data] android_list_asset_dir: getAssets missing\n");
      env->DeleteLocalRef(act_class);
      env->DeleteLocalRef(activity);
      return;
    }
  jobject java_am = env->CallObjectMethod(activity, get_assets);
  if (!java_am || env->ExceptionCheck())
    {
      if (env->ExceptionCheck())
        env->ExceptionClear();
      st_vlog("[data] android_list_asset_dir: getAssets() returned null\n");
      env->DeleteLocalRef(act_class);
      env->DeleteLocalRef(activity);
      return;
    }

  int n_ndk = 0;
  AAssetManager* mgr = AAssetManager_fromJava(env, java_am);
  if (mgr)
    {
      AAssetDir* adir = AAssetManager_openDir(mgr, norm);
      if (adir)
        {
          const char* fname;
          while ((fname = AAssetDir_getNextFileName(adir)) != NULL)
            {
              if (!string_list_contains(out, fname))
                {
                  string_list_add_item(out, fname);
                  ++n_ndk;
                }
            }
          AAssetDir_close(adir);
        }
      else
        st_vlog("[data] AAssetManager_openDir('%s') failed\n", norm);
    }
  else
    st_vlog("[data] AAssetManager_fromJava failed\n");

  /* Java list() — files + directory names (dsubdirs needs dirs). */
  int n_java = 0;
  jclass am_class = env->GetObjectClass(java_am);
  jmethodID list_mid = env->GetMethodID(
      am_class, "list", "(Ljava/lang/String;)[Ljava/lang/String;");
  if (list_mid)
    {
      jstring jpath = env->NewStringUTF(norm);
      jobjectArray listing =
          (jobjectArray)env->CallObjectMethod(java_am, list_mid, jpath);
      env->DeleteLocalRef(jpath);
      if (listing && !env->ExceptionCheck())
        {
          jsize n = env->GetArrayLength(listing);
          for (jsize i = 0; i < n; ++i)
            {
              jstring jname = (jstring)env->GetObjectArrayElement(listing, i);
              if (!jname)
                continue;
              const char* name = env->GetStringUTFChars(jname, NULL);
              if (name)
                {
                  if (!string_list_contains(out, name))
                    {
                      string_list_add_item(out, name);
                      ++n_java;
                    }
                  env->ReleaseStringUTFChars(jname, name);
                }
              env->DeleteLocalRef(jname);
            }
          env->DeleteLocalRef(listing);
        }
      else if (env->ExceptionCheck())
        env->ExceptionClear();
    }
  env->DeleteLocalRef(am_class);
  env->DeleteLocalRef(java_am);
  env->DeleteLocalRef(act_class);
  env->DeleteLocalRef(activity);

  st_vlog("[data] android_list_asset_dir('%s') → %d total (ndk files +%d, java +%d)\n",
          norm, out->num_items, n_ndk, n_java);
}

static void
android_prepare_paths(void)
{
  const char* internal = SDL_AndroidGetInternalStoragePath();
  if (!internal)
    {
      st_log("Android: SDL_AndroidGetInternalStoragePath failed: %s", SDL_GetError());
      return;
    }
  st_log("Android: internal storage = %s", internal);

  if (userdir_override.empty())
    {
      userdir_override = std::string(internal) + "/config";
      st_log("Android: userdir = %s", userdir_override.c_str());
    }

  /*
   * Read game data straight from APK assets via open_game_file() /
   * SDL_RWFromFile — no extract-to-disk. Use a non-empty logical datadir so
   * st_directory_setup does not fall through to the desktop path search
   * (which would overwrite with DATA_PREFIX "."). Path joins become
   * "./images/..."; game_file_relative() strips to "images/..." for assets.
   */
  if (datadir.empty())
    datadir = ".";
  st_log("Android: datadir = %s (APK assets via open_game_file)", datadir.c_str());

  std::string probe = datadir + "/images/status/letters-white.png";
  if (!game_file_exists(probe))
    st_log("Android: ERROR missing %s — rebuild APK with data/ in the repo root",
           probe.c_str());
  else
    st_log("Android: data probe OK (%s)", probe.c_str());
}
#endif /* __ANDROID__ */

#ifdef __EMSCRIPTEN__
/*
 * Wasm packages assets with --preload-file …@/data (see build-wasm-app.sh).
 * DATA_PREFIX is compiled as "/data". Config/saves live under a writable
 * path backed by IndexedDB (IDBFS) so progress survives page reloads.
 *
 * Requires ASYNCIFY (already on the wasm link line) so FS.syncfs can block
 * until IndexedDB finishes.
 */
void st_emscripten_fs_sync(int populate)
{
  /* populate=1: IndexedDB → MEMFS (startup). populate=0: MEMFS → IndexedDB. */
  EM_ASM({
    var populate = $0;
    Asyncify.handleSleep(function (wakeUp) {
      FS.syncfs(!!populate, function (err) {
        if (err) console.error("SuperTux IDBFS syncfs:", err);
        wakeUp();
      });
    });
  }, populate ? 1 : 0);
}

static void
emscripten_prepare_paths(void)
{
  if (userdir_override.empty())
    userdir_override = "/home/web_user/.config/supertux-milestone1";

  if (datadir.empty())
    datadir = DATA_PREFIX; /* typically "/data" */

  st_log("Emscripten: datadir = %s (MEMFS preload)", datadir.c_str());
  st_log("Emscripten: userdir = %s (IDBFS)", userdir_override.c_str());

  /* Mount IDBFS over /home/web_user so config + saves persist. */
  EM_ASM({
    try {
      FS.mkdir("/home/web_user");
    } catch (e) {}
    try {
      FS.mount(IDBFS, {}, "/home/web_user");
      console.log("SuperTux: IDBFS mounted at /home/web_user");
    } catch (e) {
      /* Already mounted on soft restart — ignore. */
      console.log("SuperTux: IDBFS mount skipped:", e);
    }
  });
  st_emscripten_fs_sync(1); /* load previous saves/config from IndexedDB */

  std::string probe = datadir + "/images/status/letters-white.png";
  if (!game_file_exists(probe))
    st_log("Emscripten: missing %s — rebuild with data/ or expect title failure",
           probe.c_str());
  else
    st_log("Emscripten: data probe OK (%s)", probe.c_str());
}
#else
void st_emscripten_fs_sync(int /*populate*/)
{
  /* no-op on native / Android */
}
#endif /* __EMSCRIPTEN__ */

/* Optional overrides from --datadir and --userdir (parsed before setup). */
void parse_path_args(int argc, char* argv[])
{
  for (int i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "--datadir") == 0 && i + 1 < argc)
        {
          datadir = argv[++i];
        }
      else if (strcmp(argv[i], "--userdir") == 0 && i + 1 < argc)
        {
          userdir_override = argv[++i];
        }
    }
}

void st_directory_setup(void)
{
#ifdef __ANDROID__
  android_prepare_paths();
#endif
#ifdef __EMSCRIPTEN__
  emscripten_prepare_paths();
#endif

  if (!userdir_override.empty())
    {
      st_dir = (char *) malloc(userdir_override.size() + 1);
      strcpy(st_dir, userdir_override.c_str());
    }
  else
    {
      std::string config_home(".");

#ifndef WIN32
      std::string home = ".";
      const char* home_c = getenv("HOME");
      if (home_c)
        {
          home = home_c;
        }

      const char* config_home_c = getenv("XDG_CONFIG_HOME");
      if (config_home_c)
        {
          config_home = config_home_c;
        }
      else
        {
          config_home = home + "/.config";
        }
#endif

      st_dir = (char *) malloc(sizeof(char) * (config_home.size() +
                                               strlen("/supertux-milestone1") + 1));
      strcpy(st_dir, config_home.c_str());
      strcat(st_dir, "/supertux-milestone1");
    }

  st_save_dir = (char *) malloc(sizeof(char) * (strlen(st_dir) + strlen("/save") + 1));

  strcpy(st_save_dir,st_dir);
  strcat(st_save_dir,"/save");

  /* Create them. In the case they exist they won't destroy anything.
     On Emscripten, create intermediate path components (MEMFS starts empty). */
#ifdef __EMSCRIPTEN__
  {
    char tmp[1024];
    size_t n = strlen(st_dir);
    if (n >= sizeof(tmp))
      n = sizeof(tmp) - 1;
    memcpy(tmp, st_dir, n);
    tmp[n] = '\0';
    for (char* p = tmp + 1; *p; ++p)
      {
        if (*p == '/')
          {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
          }
      }
    mkdir(tmp, 0755);
  }
#else
  mkdir(st_dir, 0755);
#endif
  mkdir(st_save_dir, 0755);

  char str[1024];
  snprintf(str, sizeof(str), "%s/levels", st_dir);
  mkdir(str, 0755);

  // User has not that a datadir, so we try some magic
  // Android / Emscripten set datadir in their prepare_paths helpers.
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
  if (datadir.empty())
    {
      // Detect datadir
      char exe_file[PATH_MAX];
#ifndef WIN32
      if (readlink("/proc/self/exe", exe_file, PATH_MAX) < 0)
        {
          puts("Couldn't read /proc/self/exe, using default path: " DATA_PREFIX);
          datadir = DATA_PREFIX;
        }
      else
        {
          std::string exedir = std::string(dirname(exe_file)) + "/";
          
          datadir = exedir + "../data"; // SuperTux run from source dir
          if (access(datadir.c_str(), F_OK) != 0)
            {
              datadir = exedir + "../share/supertux-milestone1"; // SuperTux run from PATH
              if (access(datadir.c_str(), F_OK) != 0) 
                { // If all fails, fall back to compiled path
        	  datadir = exedir + "./data"; // SuperTux run with data in same path as executable
        	    if (access(datadir.c_str(), F_OK) != 0)
        	    {
			 // If all fails, fall back to compiled path
                	datadir = DATA_PREFIX; 
		    }
                }
            }
        }
#else
  datadir = DATA_PREFIX;
#endif
    }
#endif /* !__ANDROID__ && !__EMSCRIPTEN__ */
  st_log("Configdir: %s", st_dir ? st_dir : "(null)");
  st_log("Datadir: %s", datadir.c_str());
}

/* Create and setup menus. */
void st_menu(void)
{
  main_menu      = new Menu();
  options_menu   = new Menu();
  options_keys_menu     = new Menu();
  options_joystick_menu = new Menu();
  options_joystick_axis_menu = new Menu();
  options_joystick_button_menu = new Menu();
  load_game_menu = new Menu();
  save_game_menu = new Menu();
  game_menu      = new Menu();
  highscore_menu = new Menu();
  contrib_menu   = new Menu();
  contrib_subset_menu   = new Menu();
  worldmap_menu  = new Menu();

  main_menu->set_pos(screen->w/2, (int)(335)+20);
  main_menu->additem(MN_GOTO, "Start Game",0,load_game_menu, MNID_STARTGAME);
  main_menu->additem(MN_GOTO, "Bonus Levels",0,contrib_menu, MNID_CONTRIB);
  main_menu->additem(MN_GOTO, "Options",0,options_menu, MNID_OPTIONMENU);
  
#ifndef GP2X
  main_menu->additem(MN_ACTION,"Level Editor",0,0, MNID_LEVELEDITOR);
#endif
  main_menu->additem(MN_ACTION,"Credits",0,0, MNID_CREDITS);
  main_menu->additem(MN_ACTION,"Quit",0,0, MNID_QUITMAINMENU);

  options_menu->additem(MN_LABEL,"Options",0,0);
  options_menu->additem(MN_HL,"",0,0);
#ifndef GP2X
#ifndef NOOPENGL
  options_menu->additem(MN_TOGGLE,"OpenGL",use_gl,0, MNID_OPENGL);
#else
  options_menu->additem(MN_DEACTIVE,"OpenGL (not supported)",use_gl, 0, MNID_OPENGL);
#endif
  options_menu->additem(MN_TOGGLE,"Fullscreen",use_fullscreen,0, MNID_FULLSCREEN);
#endif
#ifndef NOSOUND
  if(audio_device)
    {
      options_menu->additem(MN_TOGGLE,"Sound     ", use_sound,0, MNID_SOUND);
      options_menu->additem(MN_TOGGLE,"Music     ", use_music,0, MNID_MUSIC);
    }
  else
#endif
    {
      options_menu->additem(MN_DEACTIVE,"Sound     ", false,0, MNID_SOUND);
      options_menu->additem(MN_DEACTIVE,"Music     ", false,0, MNID_MUSIC);
    }
#ifdef TSCONTROL
  options_menu->additem(MN_TOGGLE,"Show Mouse",show_mouse,0, MNID_SHOWMOUSE);
#endif
  options_menu->additem(MN_TOGGLE,"Show FPS  ",show_fps,0, MNID_SHOWFPS);
#ifndef GP2X
  options_menu->additem(MN_GOTO,"Keyboard Setup",0,options_keys_menu);
#endif

  //if(use_joystick)
#ifdef GP2X
  options_menu->additem(MN_GOTO,"Joystick Move Setup",0,options_joystick_axis_menu);
  options_menu->additem(MN_GOTO,"Joystick Action Setup",0,options_joystick_button_menu);
#endif

  options_menu->additem(MN_HL,"",0,0);
  options_menu->additem(MN_BACK,"Back",0,0);
  
  options_keys_menu->additem(MN_LABEL,"Key Setup",0,0);
  options_keys_menu->additem(MN_HL,"",0,0);
  options_keys_menu->additem(MN_CONTROLFIELD,"Left move", 0,0, 0,&keymap.left);
  options_keys_menu->additem(MN_CONTROLFIELD,"Right move", 0,0, 0,&keymap.right);
  options_keys_menu->additem(MN_CONTROLFIELD,"Jump", 0,0, 0,&keymap.jump);
  options_keys_menu->additem(MN_CONTROLFIELD,"Duck", 0,0, 0,&keymap.duck);
  options_keys_menu->additem(MN_CONTROLFIELD,"Power/Run", 0,0, 0,&keymap.fire);
  options_keys_menu->additem(MN_HL,"",0,0);
  options_keys_menu->additem(MN_BACK,"Back",0,0);

#ifndef GP2X
  if(use_joystick)
    {
    options_joystick_menu->additem(MN_LABEL,"Joystick Setup",0,0);
    options_joystick_menu->additem(MN_HL,"",0,0);
    options_joystick_menu->additem(MN_CONTROLFIELD,"X axis", 0,0, 0,&joystick_keymap.x_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD,"Y axis", 0,0, 0,&joystick_keymap.y_axis);
    options_joystick_menu->additem(MN_CONTROLFIELD,"A button", 0,0, 0,&joystick_keymap.a_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"B button", 0,0, 0,&joystick_keymap.b_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"Start", 0,0, 0,&joystick_keymap.start_button);
    options_joystick_menu->additem(MN_CONTROLFIELD,"DeadZone", 0,0, 0,&joystick_keymap.dead_zone);
    options_joystick_menu->additem(MN_HL,"",0,0);
    options_joystick_menu->additem(MN_BACK,"Back",0,0);
    }
#else
    options_joystick_axis_menu->additem(MN_LABEL,"Joystick Move Setup",0,0);
    options_joystick_axis_menu->additem(MN_CONTROLFIELD,"Up", 0,0, 11,&joystick_keymap.up_button);
    options_joystick_axis_menu->additem(MN_CONTROLFIELD,"Down", 0,0, 12,&joystick_keymap.down_button);
    options_joystick_axis_menu->additem(MN_CONTROLFIELD,"Left", 0,0, 13,&joystick_keymap.left_button);
    options_joystick_axis_menu->additem(MN_CONTROLFIELD,"Right", 0,0, 14,&joystick_keymap.right_button);
    options_joystick_axis_menu->additem(MN_BACK,"Back",0,0);

    options_joystick_button_menu->additem(MN_LABEL,"Joystick Action Setup",0,0);
    options_joystick_button_menu->additem(MN_CONTROLFIELD,"Jump", 0,0, 15,&joystick_keymap.a_button);
    options_joystick_button_menu->additem(MN_CONTROLFIELD,"Shoot/Run", 0,0, 16,&joystick_keymap.b_button);
    options_joystick_button_menu->additem(MN_BACK,"Back",0,0);
#endif

  
  load_game_menu->additem(MN_LABEL,"Start Game",0,0);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0, 1);
  load_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0, 2);
  load_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0, 3);
  load_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0, 4);
  load_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0, 5);
  load_game_menu->additem(MN_HL,"",0,0);
  load_game_menu->additem(MN_BACK,"Back",0,0);

  save_game_menu->additem(MN_LABEL,"Save Game",0,0);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_DEACTIVE,"Slot 1",0,0, 1);
  save_game_menu->additem(MN_DEACTIVE,"Slot 2",0,0, 2);
  save_game_menu->additem(MN_DEACTIVE,"Slot 3",0,0, 3);
  save_game_menu->additem(MN_DEACTIVE,"Slot 4",0,0, 4);
  save_game_menu->additem(MN_DEACTIVE,"Slot 5",0,0, 5);
  save_game_menu->additem(MN_HL,"",0,0);
  save_game_menu->additem(MN_BACK,"Back",0,0);

  game_menu->additem(MN_LABEL,"Pause",0,0);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Continue",0,0,MNID_CONTINUE);
  game_menu->additem(MN_GOTO,"Options",0,options_menu);
  game_menu->additem(MN_HL,"",0,0);
  game_menu->additem(MN_ACTION,"Abort Level",0,0,MNID_ABORTLEVEL);

  worldmap_menu->additem(MN_LABEL,"Pause",0,0);
  worldmap_menu->additem(MN_HL,"",0,0);
  worldmap_menu->additem(MN_ACTION,"Continue",0,0,MNID_RETURNWORLDMAP);
  worldmap_menu->additem(MN_GOTO,"Options",0,options_menu);
  worldmap_menu->additem(MN_HL,"",0,0);
  worldmap_menu->additem(MN_ACTION,"Quit Game",0,0,MNID_QUITWORLDMAP);

  highscore_menu->additem(MN_TEXTFIELD,"Enter your name:",0,0);
}

void update_load_save_game_menu(Menu* pmenu)
{
  for(int i = 2; i < 7; ++i)
    {
      // FIXME: Insert a real savegame struct/class here instead of
      // doing string vodoo
      std::string tmp = slotinfo(i - 1);
      pmenu->item[i].kind = MN_ACTION;
      pmenu->item[i].change_text(tmp.c_str());
    }
}

bool process_load_game_menu()
{
  int slot = load_game_menu->check();

  if(slot != -1 && load_game_menu->get_item_by_id(slot).kind == MN_ACTION)
    {
      char slotfile[1024];
      snprintf(slotfile, 1024, "%s/slot%d.stsg", st_save_dir, slot);

      if (access(slotfile, F_OK) != 0)
        {
          draw_intro();
        }

      fadeout();
      WorldMapNS::WorldMap worldmap;
      
      //TODO: Define the circumstances under which BonusIsland is chosen
      worldmap.set_map_file("world1.stwm");
      worldmap.load_map();
     
      // Load the game or at least set the savegame_file variable
      worldmap.loadgame(slotfile);

      worldmap.display();
      
      Menu::set_current(main_menu);

      st_pause_ticks_stop();
      return true;
    }
  else
    {
      return false;
    }
}

/* Handle changes made to global settings in the options menu. */
void process_options_menu(void)
{
  switch (options_menu->check())
    {
    case MNID_OPENGL:
#ifndef NOOPENGL
      if(use_gl != options_menu->isToggled(MNID_OPENGL))
        {
          use_gl = !use_gl;
          st_video_setup();
        }
#else
      options_menu->get_item_by_id(MNID_OPENGL).toggled = false;
#endif
      break;
    case MNID_FULLSCREEN:
      if(use_fullscreen != options_menu->isToggled(MNID_FULLSCREEN))
        {
          use_fullscreen = !use_fullscreen;
          st_video_setup();
        }
      break;
#ifndef NOSOUND
    case MNID_SOUND:
      if(use_sound != options_menu->isToggled(MNID_SOUND))
        use_sound = !use_sound;
      break;
    case MNID_MUSIC:
      if(use_music != options_menu->isToggled(MNID_MUSIC))
        {
          use_music = !use_music;
          music_manager->enable_music(use_music);
        }
      break;
#endif
#ifdef TSCONTROL
    case MNID_SHOWMOUSE:
	  if(show_mouse != options_menu->isToggled(MNID_SHOWMOUSE))
	    show_mouse = !show_mouse;
	  break;
#endif
    case MNID_SHOWFPS:
      if(show_fps != options_menu->isToggled(MNID_SHOWFPS))
        show_fps = !show_fps;
      break;
    }
}

void st_general_setup(void)
{
  /* Seed random number generator: */

  srand(SDL_GetTicks());

#ifndef GP2X
  /* Set icon image: */

  seticon();
#endif

  /* Unicode for menu text fields (SDL1). On SDL2, call SDL_StartTextInput
     only while a text/num field is active — starting it at launch shows the
     Android soft keyboard and system bars. */
  SDL_EnableUNICODE(1);

  /* Load global images: */

#ifndef RES320X240
  white_text  = new Text(datadir + "/images/status/letters-white.png", TEXT_TEXT, 16,18);
#else
  white_text  = new Text(datadir + "/images/status/letters-white-small.png", TEXT_TEXT, 8,9);
  fadeout();
#endif


#ifndef RES320X240
  black_text  = new Text(datadir + "/images/status/letters-black.png", TEXT_TEXT, 16,18);
#else
  black_text  = new Text(datadir + "/images/status/letters-black-small.png", TEXT_TEXT, 8,9);
#endif
#ifndef RES320X240
  gold_text   = new Text(datadir + "/images/status/letters-gold.png", TEXT_TEXT, 16,18);
#else
  gold_text   = new Text(datadir + "/images/status/letters-gold-small.png", TEXT_TEXT, 8,9);
#endif
  silver_text = new Text(datadir + "/images/status/letters-silver.png", TEXT_TEXT, 16,18);
#ifndef RES320X240
  blue_text   = new Text(datadir + "/images/status/letters-blue.png", TEXT_TEXT, 16,18);
#else
  blue_text   = new Text(datadir + "/images/status/letters-blue-small.png", TEXT_TEXT, 8,9);
#endif
  red_text    = new Text(datadir + "/images/status/letters-red.png", TEXT_TEXT, 16,18);
  green_text  = new Text(datadir + "/images/status/letters-green.png", TEXT_TEXT, 16,18);
  white_text  = new Text(datadir + "/images/status/letters-white.png", TEXT_TEXT, 16,18);
  white_small_text = new Text(datadir + "/images/status/letters-white-small.png", TEXT_TEXT, 8,9);
  white_big_text   = new Text(datadir + "/images/status/letters-white-big.png", TEXT_TEXT, 20,22);
  yellow_nums = new Text(datadir + "/images/status/numbers.png", TEXT_NUM, 32,32);

  /* Load GUI/menu images: */
  checkbox = new Surface(datadir + "/images/status/checkbox.png", USE_ALPHA);
  checkbox_checked = new Surface(datadir + "/images/status/checkbox-checked.png", USE_ALPHA);
  back = new Surface(datadir + "/images/status/back.png", USE_ALPHA);
  arrow_left = new Surface(datadir + "/images/icons/left.png", USE_ALPHA);
  arrow_right = new Surface(datadir + "/images/icons/right.png", USE_ALPHA);

  /* Load the mouse-cursor */
  mouse_cursor = new MouseCursor( datadir + "/images/status/mousecursor.png",1);
  MouseCursor::set_current(mouse_cursor);
  
}

void st_general_free(void)
{

  /* Free global images: */
  delete black_text;
  delete gold_text;
  delete silver_text;
  delete white_text;
  delete blue_text;
  delete red_text;
  delete green_text;
  delete white_small_text;
  delete white_big_text;
  delete yellow_nums;

  /* Free GUI/menu images: */
  delete checkbox;
  delete checkbox_checked;
  delete back;
  delete arrow_left;
  delete arrow_right;

  /* Free mouse-cursor */
  delete mouse_cursor;
  
  /* Free menus */
  delete worldmap_menu;
  delete contrib_subset_menu;
  delete contrib_menu;
  delete highscore_menu;
  delete game_menu;
  delete save_game_menu;
  delete load_game_menu;
  delete options_joystick_button_menu;
  delete options_joystick_axis_menu;
  delete options_joystick_menu;
  delete options_keys_menu;
  delete options_menu;
  delete main_menu;
}

void st_video_setup(void)
{
  if (!platform_video_init(use_fullscreen, use_gl))
    {
      /* Options toggles can request a mode the driver rejects; fall back
         instead of aborting the whole process mid-menu. */
      fprintf(stderr,
              "Warning: video init failed (fullscreen=%d gl=%d), "
              "trying windowed software\n",
              (int)use_fullscreen, (int)use_gl);
      use_fullscreen = false;
      use_gl = false;
      if (!platform_video_init(false, false))
        {
#ifdef GP2X_VERSION
          chdir("/usr/gp2x");
          execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#endif
          exit(1);
        }
    }

  Surface::reload_all();

#ifndef GP2X_VERSION
  platform_set_caption("SuperTux " VERSION, "SuperTux");
#endif
}

void st_print_init_status(void)
{
  if (!verbose_mode)
    return;

  st_vlog("\n");
  st_vlog("========== SuperTux Milestone 1 init status ==========\n");
  st_vlog("  version      %s\n", VERSION);
  st_vlog("  platform     %s\n", platform_name());
  st_vlog("  datadir      %s\n", datadir.c_str());
  st_vlog("  config dir   %s\n", st_dir ? st_dir : "(null)");
  st_vlog("  save dir     %s\n", st_save_dir ? st_save_dir : "(null)");
  st_vlog("------------------------------------------------------\n");

  /* --- Render path (the important one) --- */
  st_vlog("  RENDER PATH  ");
  if (use_gl)
    {
#ifdef USE_GLES2
      st_vlog("OpenGL ES 2.0 (shader quads)\n");
#elif defined(NOOPENGL)
      st_vlog("software (compiled without OpenGL, use_gl was set?)\n");
#else
      st_vlog("OpenGL (desktop immediate-mode)\n");
#endif
#ifndef NOOPENGL
      {
        const char* gl_ver = (const char*)glGetString(GL_VERSION);
        const char* gl_ren = (const char*)glGetString(GL_RENDERER);
        const char* gl_ven = (const char*)glGetString(GL_VENDOR);
        st_vlog("    GL_VERSION  %s\n", gl_ver ? gl_ver : "(null)");
        st_vlog("    GL_RENDERER %s\n", gl_ren ? gl_ren : "(null)");
        st_vlog("    GL_VENDOR   %s\n", gl_ven ? gl_ven : "(null)");
      }
#endif
    }
  else
    {
#ifdef NOOPENGL
      st_vlog("software SDL surface (OpenGL not compiled in)\n");
#else
#ifdef USE_GLES2
      st_vlog("software SDL surface (GLES2 requested but not active)\n");
#else
      st_vlog("software SDL surface\n");
#endif
#endif
    }

  st_vlog("    display    %dx%d  fullscreen=%s\n",
          screen ? screen->w : 0, screen ? screen->h : 0,
          use_fullscreen ? "yes" : "no");
#ifdef USE_SDL2
  st_vlog("    SDL video  %s\n",
          SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "(none)");
#else
  {
    char driver_name[64];
    st_vlog("    SDL video  %s\n",
            SDL_VideoDriverName(driver_name, sizeof(driver_name))
              ? driver_name : "(none)");
  }
#endif

  /* Compile-time feature flags */
  st_vlog("------------------------------------------------------\n");
  st_vlog("  features (compile-time)\n");
#ifdef USE_SDL2
  st_vlog("    SDL2            built-in\n");
#else
  st_vlog("    SDL 1.2         built-in\n");
#endif
#ifdef NOOPENGL
  st_vlog("    OpenGL          not compiled (NOOPENGL)\n");
#else
#ifdef USE_GLES2
  st_vlog("    OpenGL ES 2.0   built-in (USE_GLES2)\n");
#else
  st_vlog("    OpenGL desktop  built-in (immediate-mode)\n");
#endif
#endif
#ifdef NOSOUND
  st_vlog("    audio           not compiled (NOSOUND)\n");
#else
  st_vlog("    audio           built-in (SDL_mixer)\n");
#endif
#ifdef DEBUG
  st_vlog("    DEBUG           on\n");
#else
  st_vlog("    DEBUG           off\n");
#endif

  /* Runtime subsystems */
  st_vlog("------------------------------------------------------\n");
  st_vlog("  subsystems (runtime)\n");

#ifdef NOSOUND
  st_vlog("    audio device    skipped (NOSOUND)\n");
  st_vlog("    sound effects   skipped (NOSOUND)\n");
  st_vlog("    music           skipped (NOSOUND)\n");
#else
  if (!audio_device)
    st_vlog("    audio device    skipped (unavailable or open failed)\n");
  else
    st_vlog("    audio device    initialized\n");

  if (!audio_device)
    {
      st_vlog("    sound effects   skipped (no audio device)\n");
      st_vlog("    music           skipped (no audio device)\n");
    }
  else
    {
      st_vlog("    sound effects   %s\n",
              use_sound ? "enabled" : "disabled (--disable-sound or menu)");
      st_vlog("    music           %s\n",
              use_music ? "enabled" : "disabled (--disable-music or menu)");
    }
#endif

  if (use_joystick)
    {
      const char* name = NULL;
#ifndef USE_SDL2
      name = SDL_JoystickName(joystick_num);
#else
      if (js)
        name = SDL_JoystickName(js);
#endif
      st_vlog("    joystick        initialized (index %d%s%s%s)\n",
              joystick_num,
              name ? ", \"" : "",
              name ? name : "",
              name ? "\"" : "");
    }
  else
    {
      st_vlog("    joystick        skipped (none available or open failed)\n");
    }

  st_vlog("    keyboard        active\n");
  st_vlog("======================================================\n\n");
}

void st_video_setup_sdl(void)
{
  /* Kept for menu toggles that re-init software mode */
  if (!platform_video_init(use_fullscreen, false))
    {
      st_log("FATAL: software video init failed: %s", SDL_GetError());
      exit(1);
    }
}

void st_video_setup_gl(void)
{
  if (!platform_video_init(use_fullscreen, true))
    {
      st_log("FATAL: GL video init failed: %s", SDL_GetError());
      exit(1);
    }
}

void st_joystick_setup(void)
{

  /* Init Joystick: */

  use_joystick = true;
  js = NULL;

  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
    {
      fprintf(stderr, "Warning: I could not initialize joystick!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());

      use_joystick = false;
      if (verbose_mode)
        st_vlog("[joy] SDL_InitSubSystem(JOYSTICK) failed: %s\n", SDL_GetError());
    }
  else
    {
      int njoy = SDL_NumJoysticks();
      if (verbose_mode)
        {
          st_vlog("[joy] SDL_NumJoysticks() = %d (want index %d)\n",
                  njoy, joystick_num);
          for (int i = 0; i < njoy; ++i)
            {
#ifdef USE_SDL2
              const char* jname = SDL_JoystickNameForIndex(i);
#else
              const char* jname = SDL_JoystickName(i);
#endif
              st_vlog("[joy]   [%d] \"%s\"\n", i, jname ? jname : "(unnamed)");
            }
        }

      /* Open joystick: */
      if (njoy <= 0)
        {
          if (verbose_mode)
            st_vlog("[joy] no joysticks available — skipping\n");
          else
            fprintf(stderr, "Warning: No joysticks are available.\n");

          use_joystick = false;
        }
      else
        {
          js = SDL_JoystickOpen(joystick_num);

          if (js == NULL)
            {
              fprintf(stderr, "Warning: Could not open joystick %d.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", joystick_num, SDL_GetError());

              use_joystick = false;
              if (verbose_mode)
                st_vlog("[joy] open index %d failed: %s\n",
                        joystick_num, SDL_GetError());
            }
#ifndef GP2X
          else
            {
              int naxes = SDL_JoystickNumAxes(js);
              int nbuttons = SDL_JoystickNumButtons(js);
#ifdef USE_SDL2
              const char* jname = SDL_JoystickName(js);
#else
              const char* jname = SDL_JoystickName(joystick_num);
#endif
              if (verbose_mode)
                st_vlog("[joy] opened index %d \"%s\" (%d axes, %d buttons)\n",
                        joystick_num,
                        jname ? jname : "(unnamed)",
                        naxes, nbuttons);

              if (naxes < 2)
                {
                  fprintf(stderr,
                          "Warning: Joystick does not have enough axes!\n");
                  SDL_JoystickClose(js);
                  js = NULL;
                  use_joystick = false;
                  if (verbose_mode)
                    st_vlog("[joy] rejected: need >= 2 axes (have %d)\n", naxes);
                }
              else if (nbuttons < 2)
                {
                  fprintf(stderr,
                          "Warning: "
                          "Joystick does not have enough buttons!\n");
                  /* Close rejected device so it cannot keep sending events. */
                  SDL_JoystickClose(js);
                  js = NULL;
                  use_joystick = false;
                  if (verbose_mode)
                    st_vlog("[joy] rejected: need >= 2 buttons (have %d)\n",
                            nbuttons);
                }
            }
#endif
        }
    }

  if (verbose_mode)
    st_vlog("[joy] use_joystick=%s js=%p\n",
            use_joystick ? "true" : "false", (void*)js);
}

void st_sdl_init(void)
{
#ifdef USE_GLES2
  /* Must be set before the first video init: X11/GLX cannot create ES
     contexts; EGL can. Also prefer a native GLES driver when available. */
#ifdef SDL_HINT_VIDEO_X11_FORCE_EGL
  SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1");
#endif
#ifdef SDL_HINT_OPENGL_ES_DRIVER
  SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif
  if (verbose_mode)
    st_vlog("[init] GLES2: set X11_FORCE_EGL and OPENGL_ES_DRIVER hints\n");
#endif

#ifdef __ANDROID__
  /* Must run before SDL_Init: otherwise KEYCODE_BACK finishes the Activity
     and the game never sees an event. With the trap on, Back becomes
     SDL_SCANCODE_AC_BACK (SDLK_AC_BACK) for st_is_escape_event(). */
#ifdef SDL_HINT_ANDROID_TRAP_BACK_BUTTON
  SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
#endif
  SDL_setenv("SDL_ANDROID_TRAP_BACK_BUTTON", "1", 1);
  if (verbose_mode)
    st_vlog("[init] Android: trap BACK as SDL_SCANCODE_AC_BACK\n");
#endif

  if (verbose_mode)
    st_vlog("[init] SDL_Init(VIDEO|TIMER)...\n");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    {
      st_log("Error: SDL_Init failed: %s", SDL_GetError());
      exit(1);
    }
  if (verbose_mode)
    {
#ifdef USE_SDL2
      SDL_version linked;
      SDL_GetVersion(&linked);
      st_vlog("[init] SDL_Init ok (headers %d.%d.%d, linked %d.%d.%d)\n",
              SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
              linked.major, linked.minor, linked.patch);
#else
      const SDL_version* linked = SDL_Linked_Version();
      st_vlog("[init] SDL_Init ok (headers %d.%d.%d, linked %d.%d.%d)\n",
              SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
              linked->major, linked->minor, linked->patch);
#endif
    }
}

void st_audio_setup(void)
{
#ifdef NOSOUND
  if (verbose_mode)
    st_vlog("[audio] skipped (compiled with NOSOUND)\n");
  return;
#else

  /* Init SDL Audio silently even if --disable-sound.
     On device failure only clear audio_device — never clear use_sound /
     use_music. Those are user preferences written by saveconfig(); wiping
     them on a transient OpenSL failure permanently mutes the next launch
     even when Mix_OpenAudio would succeed (common Android device split). */

  if (!audio_device)
    {
      if (verbose_mode)
        st_vlog("[audio] device open skipped (--disable-sound or prior failure)\n");
    }
  else if (audio_device)
    {
      if (verbose_mode)
        st_vlog("[audio] SDL_InitSubSystem(AUDIO)... (pref SFX %s music %s)\n",
                use_sound ? "on" : "off",
                use_music ? "on" : "off");
      if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        {
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not initialize audio!\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
            }
          audio_device = false;
          if (verbose_mode)
            st_vlog("[audio] subsystem failed — prefs unchanged (SFX %s music %s)\n",
                    use_sound ? "on" : "off",
                    use_music ? "on" : "off");
        }
      else if (verbose_mode)
        {
          st_vlog("[audio] subsystem ready (SFX %s, music %s)\n",
                  use_sound ? "on" : "off",
                  use_music ? "on" : "off");
        }
    }

#ifdef GP2X
	//This is from the GP2X patch (without the ifdefs)
    audio_device = true;
#endif
    
  /* Open sound silently regardless of "use_sound" (device probe). */

  if (audio_device)
    {
#ifndef GP2X    
      if (open_audio(44100, AUDIO_S16, 2, 2048) < 0)
#else
      if (open_audio(44100, AUDIO_S16, 1, 1024) < 0)
#endif      
        {
          if (use_sound || use_music)
            {
              fprintf(stderr,
                      "\nWarning: I could not set up audio for 44100 Hz "
                      "16-bit stereo.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
            }
          audio_device = false;
          if (verbose_mode)
            st_vlog("[audio] Mix_OpenAudio failed — prefs unchanged\n");
        }
      else if (verbose_mode)
        {
          st_vlog("[audio] Mix_OpenAudio ok (44100 Hz) playback SFX %s music %s\n",
                  use_sound ? "on" : "off",
                  use_music ? "on" : "off");
          if (!use_sound && !use_music)
            st_vlog("[audio] both prefs off — enable in Options or clear config\n");
        }
    }

#endif
}


/* --- SHUTDOWN --- */

void st_shutdown(void)
{
#ifndef NOSOUND
  close_audio();
#endif
  platform_video_shutdown();
  SDL_Quit();
  saveconfig();
#ifdef GP2X
    chdir("/usr/gp2x");
    execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);    
#endif

}

/* --- ABORT! --- */

void st_abort(const std::string& reason, const std::string& details)
{
  st_log("Error: %s %s", reason.c_str(), details.c_str());
  st_shutdown();
  abort();
}

/* Set Icon (private) */

void seticon(void)
{
#ifndef GP2X
  SDL_Surface * icon = 0;
  std::string icon_path = datadir + "/images/icon.png";

  /* open_game_file so Android can read the icon from APK assets. */
  {
    SDL_RWops* rw = open_game_file(icon_path);
    if (rw)
      icon = IMG_Load_RW(rw, 1);
  }
  if (icon == NULL)
    {
      /* Window icon is optional — do not kill the process. */
      st_log("Warning: could not load icon %s: %s",
             icon_path.c_str(), IMG_GetError());
      return;
    }

  platform_set_icon(icon);
  SDL_FreeSurface(icon);
#endif
}


/* Parse command-line arguments: */

void parseargs(int argc, char * argv[])
{
  int i;

  loadconfig();

  /* Parse arguments: */

  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "--fullscreen") == 0 ||
          strcmp(argv[i], "-f") == 0)
        {
          /* Use full screen: */

          use_fullscreen = true;
        }
      else if (strcmp(argv[i], "--window") == 0 ||
               strcmp(argv[i], "-w") == 0)
        {
          /* Use window mode: */

          use_fullscreen = false;
        }      
      else if (strcmp(argv[i], "--joystick") == 0 || strcmp(argv[i], "-j") == 0)
        {
          assert(i+1 < argc);
          joystick_num = atoi(argv[++i]);
        }
      else if (strcmp(argv[i], "--joymap") == 0)
        {
#ifndef GP2X
          assert(i+1 < argc);
          if (sscanf(argv[++i],
                     "%d:%d:%d:%d:%d", 
                     &joystick_keymap.x_axis, 
                     &joystick_keymap.y_axis, 
                     &joystick_keymap.a_button, 
                     &joystick_keymap.b_button, 
                     &joystick_keymap.start_button) != 5)
            {
              puts("Warning: Invalid or incomplete joymap, should be: 'XAXIS:YAXIS:A:B:START'");
            }
          else
            {
              std::cout << "Using new joymap:\n"
                        << "  X-Axis:       " << joystick_keymap.x_axis << "\n"
                        << "  Y-Axis:       " << joystick_keymap.y_axis << "\n"
                        << "  A-Button:     " << joystick_keymap.a_button << "\n"
                        << "  B-Button:     " << joystick_keymap.b_button << "\n"
                        << "  Start-Button: " << joystick_keymap.start_button << std::endl;
            }
#endif
        }
      else if (strcmp(argv[i], "--leveleditor") == 0)
        {
          launch_leveleditor_mode = true;
        }
      else if (strcmp(argv[i], "--datadir") == 0)
        {
          /* Also handled in parse_path_args() before directory setup. */
          if (i + 1 >= argc)
            {
              fprintf(stderr, "Error: --datadir requires a directory argument\n");
              usage(argv[0], 1);
            }
          datadir = argv[++i];
        }
      else if (strcmp(argv[i], "--userdir") == 0)
        {
          /* Handled in parse_path_args() before directory setup; skip value. */
          if (i + 1 >= argc)
            {
              fprintf(stderr, "Error: --userdir requires a directory argument\n");
              usage(argv[0], 1);
            }
          ++i;
        }
      else if (strcmp(argv[i], "--show-fps") == 0)
        {
          /* Use full screen: */

          show_fps = true;
        }
      else if (strcmp(argv[i], "--opengl") == 0 ||
               strcmp(argv[i], "-gl") == 0)
        {
#ifndef NOOPENGL
          /* Use OpengGL: */

          use_gl = true;
#endif
        }
      else if (strcmp(argv[i], "--sdl") == 0)
          {
            use_gl = false;
          }
            else if (strcmp(argv[i], "--verbose") == 0
               || strcmp(argv[i], "-v") == 0)
        {
          verbose_mode = true;
          st_vlog("[verbose] enabled — will report render path and subsystem status\n");
        }
      else if (strcmp(argv[i], "--usage") == 0)
        {
          /* Show usage: */

          usage(argv[0], 0);
        }
      else if (strcmp(argv[i], "--version") == 0)
        {
          /* Show version: */
          printf("SuperTux " VERSION "\n");
          exit(0);
        }
#ifndef NOSOUND
      else if (strcmp(argv[i], "--disable-sound") == 0)
        {
          /* Disable the compiled in sound feature */
          printf("Sounds disabled \n");
          use_sound = false;
          audio_device = false;
        }
      else if (strcmp(argv[i], "--disable-music") == 0)
        {
          /* Disable the compiled in sound feature */
          printf("Music disabled \n");
          use_music = false;
        }
#endif
      else if (strcmp(argv[i], "--debug-mode") == 0)
        {
          /* Enable the debug-mode */
          debug_mode = true;

        }
      else if (strcmp(argv[i], "--help") == 0)
        {     /* Show help: */
          puts("Super Tux " VERSION "\n"
               "  Please see the file \"README.txt\" for more details.\n");
          printf("Usage: %s [OPTIONS] FILENAME\n\n", argv[0]);
          puts("Display Options:\n"
               "  -w, --window        Run in window mode.\n"
               "  -f, --fullscreen    Run in fullscreen mode.\n"
               "  -gl, --opengl       If opengl support was compiled in, this will enable\n"
               "                      the OpenGL mode.\n"
               "  --sdl               Use non-opengl renderer\n"
               "  -v, --verbose       Log render path, video setup, and which\n"
               "                      subsystems were initialized or skipped\n"
               "\n"
               "Sound Options:\n"
               "  --disable-sound     If sound support was compiled in,  this will\n"
               "                      disable sound for this session of the game.\n"
               "  --disable-music     Like above, but this will disable music.\n"
               "\n"
               "Misc Options:\n"
               "  -j, --joystick NUM  Use joystick NUM (default: 0)\n" 
               "  --joymap XAXIS:YAXIS:A:B:START\n"
               "  --leveleditor       Opens the leveleditor in a file. (Only works when a file is provided.)\n"
               "                      Define how joystick buttons and axis should be mapped\n"
               "  --datadir DIR       Load game datafiles from DIR (default: automatic)\n"
               "  --userdir DIR       Load config files and store savegames in DIR\n"
               "  --debug-mode        Enables the debug-mode, which is useful for developers.\n"
               "  --help              Display a help message summarizing command-line\n"
               "                      options, license and game controls.\n"
               "  --usage             Display a brief message summarizing command-line options.\n"
               "  --version           Display the version of SuperTux you're running.\n\n"
               );
          exit(0);
        }
      else if (argv[i][0] != '-')
        {
          level_startup_file = argv[i];
        }
      else
        {
          /* Unknown - complain! */

          usage(argv[0], 1);
        }
    }

#ifdef __ANDROID__
  /* Phone/tablet: fullscreen; GLES2 is the default accelerated path.
     Verbose stays on so render path / subsystem status shows in logcat.
     On-screen virtual gamepad for play without a controller. */
  use_fullscreen = true;
  verbose_mode = true;
#ifdef USE_GLES2
  use_gl = true;
#endif
  touch_controls_set_enabled(true);
#endif
}


/* Display usage: */

void usage(char * prog, int ret)
{
  FILE * fi;


  /* Determine which stream to write to: */

  if (ret == 0)
    fi = stdout;
  else
    fi = stderr;


  /* Display the usage message: */

  fprintf(fi, "Usage: %s [--fullscreen] [--opengl] [--disable-sound] [--disable-music] [--debug-mode] | [--usage | --help | --version] FILENAME\n",
          prog);


  /* Quit! */

  exit(ret);
}


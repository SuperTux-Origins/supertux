//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "supertux/main.hpp"


#include <SDL_ttf.h>
#include <physfs.h>

#ifdef __ANDROID__
#  include <unistd.h>
#endif

#if !defined(WIN32) && !defined(EMSCRIPTEN)
#  include <xdgcpp/xdg.h>
#endif

#include <version.h>

#include "math/random.hpp"
#include "object/player.hpp"
#include "object/spawnpoint.hpp"
#include "supertux/error_handler.hpp"
#include "supertux/game_session.hpp"
#include "supertux/level.hpp"
#include "supertux/screen_fade.hpp"
#include "supertux/sector.hpp"
#include "supertux/title_screen.hpp"
#include "util/file_system.hpp"
#include "util/timelog.hpp"
#include "video/sdl_surface.hpp"
#include "worldmap/worldmap.hpp"
#include "worldmap/worldmap_screen.hpp"

static Timelog s_timelog;

ConfigSubsystem::ConfigSubsystem() :
  m_config()
{
  g_config = &m_config;
  try {
    m_config.load();
  }
  catch(std::exception const& e)
  {
    log_info("Couldn't load config file: {}, using default settings", e.what());
  }

  // init random number stuff
  gameRandom.seed(m_config.random_seed);
  graphicsRandom.seed(0);
  //const char *how = config->random_seed? ", user fixed.": ", from time().";
  //log_info("Using random seed {}{}", config->random_seed, how);
}

ConfigSubsystem::~ConfigSubsystem()
{
  try
  {
    m_config.save();
  }
  catch(std::exception& e)
  {
    log_warning("Error saving config: {}", e.what());
  }
}

Main::Main() :
  m_physfs_subsystem(),
  m_config_subsystem(),
  m_sdl_subsystem(),
  m_console_buffer(),
  m_input_manager(),
  m_video_system(),
  m_ttf_surface_manager(),
  m_sound_manager(),
  m_squirrel_virtual_machine(),
  m_tile_manager(),
  m_sprite_manager(),
  m_resources(),
  m_console(),
  m_game_manager(),
  m_screen_manager(),
  m_savegame()
{
}

PhysfsSubsystem::PhysfsSubsystem(char const* argv0,
                                 std::optional<std::string> forced_datadir,
                                 std::optional<std::string> forced_userdir) :
  m_forced_datadir(std::move(forced_datadir)),
  m_forced_userdir(std::move(forced_userdir))
{
  if (!PHYSFS_init(argv0))
  {
    std::stringstream msg;
    msg << "Couldn't initialize physfs: " << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
    throw std::runtime_error(msg.str());
  }
  else
  {
    // allow symbolic links
    PHYSFS_permitSymbolicLinks(1);

    find_userdir();
    find_datadir();
  }
}

void PhysfsSubsystem::find_datadir() const
{
#ifndef __EMSCRIPTEN__
  if (char const* assetpack = getenv("ANDROID_ASSET_PACK_PATH"))
  {
    // Android asset pack has a hardcoded prefix for data files, and PhysFS cannot strip it, so we mount an archive inside an archive
    if (!PHYSFS_mount(std::filesystem::canonical(assetpack).string().c_str(), nullptr, 1))
    {
      log_warning("Couldn't add '{}' to physfs searchpath: {}", assetpack, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      return;
    }

    PHYSFS_File* data = PHYSFS_openRead("assets/data.zip");
    if (!data)
    {
      log_warning("Couldn't open assets/data.zip inside '{}' : {}", assetpack, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
      return;
    }

    if (!PHYSFS_mountHandle(data, "assets/data.zip", nullptr, 1))
    {
      log_warning("Couldn't add assets/data.zip inside '{}' to physfs searchpath: {}", assetpack, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }

    return;
  }

  std::string datadir;
  if (m_forced_datadir)
  {
    datadir = *m_forced_datadir;
  }
  else if (char const* env_datadir = getenv("SUPERTUX_ORIGNS_DATA_DIR"))
  {
    datadir = env_datadir;
  }
  else if (char const* env_datadir3 = getenv("ANDROID_MY_OWN_APP_FILE"))
  {
    datadir = env_datadir3;
  }
  else
  {
    // check if we run from source dir
    char* basepath_c = SDL_GetBasePath();
    std::string basepath = basepath_c ? basepath_c : "./";
    SDL_free(basepath_c);

    if (FileSystem::exists(FileSystem::join(BUILD_DATA_DIR, "credits.stxt")))
    {
      datadir = BUILD_DATA_DIR;
      // Add config dir for supplemental files
      PHYSFS_mount(std::filesystem::canonical(BUILD_CONFIG_DATA_DIR).string().c_str(), nullptr, 1);
    }
    else
    {
      // if the game is not run from the source directory, try to find
      // the global install location
      datadir = basepath.substr(0, basepath.rfind(INSTALL_SUBDIR_BIN));
      datadir = FileSystem::join(datadir, INSTALL_SUBDIR_SHARE);
    }
  }

  // An additional .string() call is necessary as Windows returns const wchar_t* for .c_str()
  if (!PHYSFS_mount(std::filesystem::canonical(datadir).string().c_str(), nullptr, 1))
  {
    log_warning("Couldn't add '{}' to physfs searchpath: {}", datadir, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
  }
#else
  if (!PHYSFS_mount(BUILD_CONFIG_DATA_DIR, nullptr, 1))
  {
    log_warning("Couldn't add '{}' to physfs searchpath: {}", BUILD_CONFIG_DATA_DIR, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
  }
#endif
}

void PhysfsSubsystem::find_userdir() const
{
  std::string userdir;
  if (m_forced_userdir)
  {
    userdir = *m_forced_userdir;
  }
  else if (char const* env_userdir = getenv("SUPERTUX_ORIGNS_USER_DIR"))
  {
    userdir = env_userdir;
  }
  else
  {
#if defined(WIN32)
    userdir = PHYSFS_getPrefDir("SuperTux","supertux-origins");
#elif defined(EMSCRIPTEN)
    userdir = "/home/web_user/.local/share/supertux-origins/";
#else
    userdir = xdg::config().home() / "supertux-origins";
#endif
  }

  if (!FileSystem::is_directory(userdir))
  {
    FileSystem::mkdir(userdir);
    log_info("Created SuperTux userdir: {}", userdir);
  }

#ifdef EMSCRIPTEN
  EM_ASM({
    try {
      FS.mount(IDBFS, {}, "/home/web_user/.local/share/supertux-origins/");
      FS.syncfs(true, (err) => { console.log(err); });
    } catch(err) {}
  }, 0); // EM_ASM is a variadic macro and Clang requires at least 1 value for the variadic argument
#endif

  if (!PHYSFS_setWriteDir(userdir.c_str()))
  {
    std::ostringstream msg;
    msg << "Failed to use userdir directory '"
        <<  userdir << "': errorcode: " << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
    throw std::runtime_error(msg.str());
  }

  PHYSFS_mount(userdir.c_str(), nullptr, 0);
}

void PhysfsSubsystem::print_search_path()
{
  char const* writedir = PHYSFS_getWriteDir();
  log_info("PhysfsWriteDir: {}", (writedir ? writedir : "(null)"));
  log_info("PhysfsSearchPath:");
  char** searchpath = PHYSFS_getSearchPath();
  for (char** i = searchpath; *i != nullptr; ++i)
  {
    log_info("  {}", *i);
  }
  PHYSFS_freeList(searchpath);
}

PhysfsSubsystem::~PhysfsSubsystem()
{
  PHYSFS_deinit();
}

SDLSubsystem::SDLSubsystem()
{
  Uint32 flags = SDL_INIT_TIMER | SDL_INIT_VIDEO;
#ifndef UBUNTU_TOUCH
  flags |= SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;
#endif
  if (SDL_Init(flags) < 0)
  {
    std::stringstream msg;
    msg << "Couldn't initialize SDL: " << SDL_GetError();
    throw std::runtime_error(msg.str());
  }

  if (TTF_Init() < 0)
  {
    std::stringstream msg;
    msg << "Couldn't initialize SDL TTF: " << SDL_GetError();
    throw std::runtime_error(msg.str());
  }

  // just to be sure
  atexit(TTF_Quit);
  atexit(SDL_Quit);
}

SDLSubsystem::~SDLSubsystem()
{
  TTF_Quit();
  SDL_Quit();
}

void
Main::init_video()
{
  VideoSystem::current()->set_title("SuperTux " PACKAGE_VERSION);

  char const* icon_fname = "images/engine/icons/supertux-256x256.png";

  SDLSurfacePtr icon = SDLSurface::from_file(icon_fname);
  VideoSystem::current()->set_icon(*icon);

  SDL_ShowCursor(g_config->custom_mouse_cursor ? 0 : 1);

  log_info("{} Window: {} Fullscreen: {}@{} Area: {}", (g_config->use_fullscreen?"fullscreen ":"window "), g_config->window_size, g_config->fullscreen_size, g_config->fullscreen_refresh_rate, g_config->aspect_size);
}

void
Main::launch_game(CommandLineArguments const& args)
{
  m_sdl_subsystem.reset(new SDLSubsystem());
  m_console_buffer.reset(new ConsoleBuffer());
#ifdef ENABLE_TOUCHSCREEN_SUPPORT
  if (getenv("ANDROID_TV")) {
    g_config->mobile_controls = false;
  }
#endif

  s_timelog.log("controller");
  m_input_manager.reset(new InputManager(g_config->keyboard_config, g_config->joystick_config));

  s_timelog.log("commandline");

#ifndef EMSCRIPTEN
  auto video = g_config->video;
  s_timelog.log("video");

  m_video_system = VideoSystem::create(video);
#else
  // Force SDL for WASM builds, as OpenGL is reportedly slow on some devices
  m_video_system = VideoSystem::create(VideoSystem::VIDEO_SDL);
#endif
  init_video();

  m_ttf_surface_manager.reset(new TTFSurfaceManager());

  s_timelog.log("audio");
  m_sound_manager.reset(new SoundManager());
  m_sound_manager->enable_sound(g_config->sound_enabled);
  m_sound_manager->enable_music(g_config->music_enabled);
  m_sound_manager->set_sound_volume(g_config->sound_volume);
  m_sound_manager->set_music_volume(g_config->music_volume);

  s_timelog.log("scripting");
  m_squirrel_virtual_machine.reset(new SquirrelVirtualMachine(g_config->enable_script_debugger));

  s_timelog.log("resources");
  m_tile_manager.reset(new TileManager());
  m_sprite_manager.reset(new SpriteManager());
  m_resources.reset(new Resources());

  m_console.reset(new Console(*m_console_buffer));

  s_timelog.log(nullptr);

  m_savegame = std::make_unique<Savegame>(std::string());

  m_game_manager.reset(new GameManager());
  m_screen_manager.reset(new ScreenManager(*m_video_system, *m_input_manager));

  if (!args.filenames.empty())
  {
    for(auto const& start_level : args.filenames)
    {
      // we have a normal path specified at commandline, not a physfs path.
      // So we simply mount that path here...
      std::string dir = FileSystem::dirname(start_level);
      const std::string filename = FileSystem::basename(start_level);
      const std::string fileProtocol = "file://";
      const std::string::size_type position = dir.find(fileProtocol);
      if (position != std::string::npos) {
        dir = dir.replace(position, fileProtocol.length(), "");
      }
      log_debug("Adding dir: {}", dir);
      PHYSFS_mount(dir.c_str(), nullptr, true);

      if (start_level.ends_with(".stwm"))
      {
        m_screen_manager->push_screen(std::make_unique<worldmap::WorldMapScreen>(
                                     std::make_unique<worldmap::WorldMap>(filename, *m_savegame)));
      }
      else
      { // launch game
        std::unique_ptr<GameSession> session (
          new GameSession(filename, *m_savegame));

        g_config->random_seed = session->get_demo_random_seed(g_config->start_demo);
        gameRandom.seed(g_config->random_seed);
        graphicsRandom.seed(0);

        if (args.sector || args.spawnpoint)
        {
          std::string sectorname = args.sector.value_or("main");

          auto const& spawnpoints = session->get_current_sector().get_objects_by_type<SpawnPointMarker>();
          std::string default_spawnpoint = (spawnpoints.begin() != spawnpoints.end()) ?
            "" : spawnpoints.begin()->get_name();
          std::string spawnpointname = args.spawnpoint.value_or(default_spawnpoint);

          session->set_start_point(sectorname, spawnpointname);
          session->restart_level();
        }

        if (g_config->tux_spawn_pos)
        {
          // FIXME: Specify start pos for multiple players
          session->get_current_sector().get_players()[0]->set_pos(*g_config->tux_spawn_pos);
        }

        if (!g_config->start_demo.empty())
          session->play_demo(g_config->start_demo);

        if (!g_config->record_demo.empty())
          session->record_demo(g_config->record_demo);
        m_screen_manager->push_screen(std::move(session));
      }
    }
  }
  else
  {
    m_screen_manager->push_screen(std::make_unique<TitleScreen>(*m_savegame));
  }

  m_screen_manager->run();
}

int
Main::run(int argc, char** argv)
{
  // First and foremost, set error handlers (to print stack trace on SIGSEGV, etc.)
  ErrorHandler::set_handlers();

#ifdef __EMSCRIPTEN__
  init_emscripten();
#endif

#ifdef WIN32
	//SDL is used instead of PHYSFS because both create the same path in app data
	//However, PHYSFS is not yet initizlized, and this should be run before anything is initialized
	std::string prefpath = SDL_GetPrefPath("SuperTux", "supertux-origins");

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	//All this conversion stuff is necessary to make this work for internationalized usernames
	std::string outpath = prefpath + "/console.out";
	std::wstring w_outpath = converter.from_bytes(outpath);
	_wfreopen(w_outpath.c_str(), L"a", stdout);

	std::string errpath = prefpath + "/console.err";
	std::wstring w_errpath = converter.from_bytes(errpath);
	_wfreopen(w_errpath.c_str(), L"a", stderr);
#endif

  int result = 0;

  try
  {
    CommandLineArguments args;
    try
    {
      args.parse_args(argc, argv);
      logmich::g_logger.set_log_level(args.get_log_level());
    }
    catch(std::exception const& err)
    {
      std::cout << "Error: " << err.what() << std::endl;
      return EXIT_FAILURE;
    }

    m_physfs_subsystem.reset(new PhysfsSubsystem(argv[0], args.datadir, args.userdir));
    m_physfs_subsystem->print_search_path();

    s_timelog.log("config");
    m_config_subsystem.reset(new ConfigSubsystem());
    args.merge_into(*g_config);

    s_timelog.log("tinygettext");

    switch (args.get_action())
    {
      case CommandLineArguments::PRINT_VERSION:
        args.print_version();
        return 0;

      case CommandLineArguments::PRINT_HELP:
        args.print_help(argv[0]);
        return 0;

      case CommandLineArguments::PRINT_DATADIR:
        args.print_datadir();
        return 0;

      case CommandLineArguments::PRINT_ACKNOWLEDGEMENTS:
        args.print_acknowledgements();
        return 0;

      default:
        launch_game(args);
        break;
    }
  }
  catch(std::exception const& e)
  {
    log_fatal("Unexpected exception: {}", e.what());
    result = 1;
  }
  catch(...)
  {
    log_fatal("Unexpected exception");
    result = 1;
  }

#ifdef __ANDROID__
  // SDL2 keeps shared libraries loaded after the app is closed,
  // when we launch the app again the static initializers will run twice and crash the app.
  // So we just need to terminate the app process 'gracefully', without running destructors or atexit() functions.
  _exit(result);
#endif

  return result;
}

/* EOF */

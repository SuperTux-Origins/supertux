//  SuperTux -  A Jump'n Run
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

#include "supertux/gameconfig.hpp"

#include <ctime>

#include "config.h"

#include "supertux/colorscheme.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"
#include "util/log.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

Config::Config() :
  profile(1),
  profiles(),
  fullscreen_size(0, 0),
  fullscreen_refresh_rate(0),
  window_size(1280, 800),
  window_resizable(true),
  aspect_size(0, 0), // auto detect
#ifdef __EMSCRIPTEN__
  fit_window(true),
#endif
  magnification(0.0f),
  // Ubuntu Touch supports windowed apps
#ifdef __ANDROID__
  use_fullscreen(true),
#else
  use_fullscreen(false),
#endif
  video(VideoSystem::VIDEO_AUTO),
  try_vsync(true),
  show_fps(false),
  show_player_pos(false),
  show_controller(false),
  sound_enabled(true),
  music_enabled(true),
  sound_volume(100),
  music_volume(50),
  random_seed(0), // set by time(), by default (unless in config)
  enable_script_debugger(false),
  start_demo(),
  record_demo(),
  tux_spawn_pos(),
  locale(),
  keyboard_config(),
  joystick_config(),
  mobile_controls(SDL_GetNumTouchDevices() > 0),
  m_mobile_controls_scale(1),
  addons(),
  developer_mode(false),
  christmas_mode(false),
  transitions_enabled(true),
  confirmation_dialog(false),
  pause_on_focusloss(true),
  custom_mouse_cursor(true),
  hide_editor_levelnames(false),
  notifications(),
  menubackcolor(ColorScheme::Menu::back_color),
  menufrontcolor(ColorScheme::Menu::front_color),
  menuhelpbackcolor(ColorScheme::Menu::help_back_color),
  menuhelpfrontcolor(ColorScheme::Menu::help_front_color),
  labeltextcolor(ColorScheme::Menu::label_color),
  activetextcolor(ColorScheme::Menu::active_color),
  hlcolor(ColorScheme::Menu::hl_color),
  editorcolor(),
  editorhovercolor(),
  editorgrabcolor(),
  menuroundness(16.f),
  editor_selected_snap_grid_size(3),
  editor_render_grid(true),
  editor_snap_to_grid(true),
  editor_render_background(true),
  editor_render_lighting(false),
  editor_autotile_mode(false),
  editor_autotile_help(true),
  editor_autosave_frequency(5),
  multiplayer_auto_manage_players(true),
  multiplayer_multibind(false),
#if SDL_VERSION_ATLEAST(2, 0, 9)
  multiplayer_buzz_controllers(true),
#else
  // Will be loaded and saved anyways, to retain the setting. This is helpful
  // for users who frequently switch between versions compiled with a newer SDL
  // and those with an older SDL; they won't have to check the setting each time
  multiplayer_buzz_controllers(false),
#endif
  repository_url()
{
}

void
Config::load()
{
#ifdef __EMSCRIPTEN__
  EM_ASM({
    supertux_loadFiles();
  }, 0); // EM_ASM is a variadic macro and Clang requires at least 1 value for the variadic argument
#endif

  auto doc = load_reader_document("config");
  auto root = doc.get_root();
  if (root.get_name() != "supertux-config")
  {
    throw std::runtime_error("File is not a supertux-config file");
  }

  auto config_mapping = root.get_mapping();
  config_mapping.read("profile", profile);
  ReaderCollection config_profiles_mapping;
  if (config_mapping.read("profiles", config_profiles_mapping))
  {
    for (auto const& profile_node : config_profiles_mapping.get_objects())
    {
      if (profile_node.get_name() == "profile")
      {
        auto current_profile = profile_node.get_mapping();

        int id;
        std::string name;
        if (current_profile.read("id", id) &&
            current_profile.read("name", name))
        {
          profiles.push_back({id, name});
        }
      }
      else
      {
        log_warning << "Unknown token in config file: " << profile_node.get_name() << std::endl;
      }
    }
  }
  config_mapping.read("show_fps", show_fps);
  config_mapping.read("show_player_pos", show_player_pos);
  config_mapping.read("show_controller", show_controller);
  config_mapping.read("developer", developer_mode);
  config_mapping.read("confirmation_dialog", confirmation_dialog);
  config_mapping.read("pause_on_focusloss", pause_on_focusloss);
  config_mapping.read("custom_mouse_cursor", custom_mouse_cursor);

  ReaderCollection config_notifications_mapping;
  if (config_mapping.read("notifications", config_notifications_mapping))
  {
    for (auto const& notification_node : config_notifications_mapping.get_objects())
    {
      if (notification_node.get_name() == "notification")
      {
        auto notification = notification_node.get_mapping();

        std::string id;
        bool disabled = false;
        if (notification.read("id", id) &&
            notification.read("disabled", disabled))
        {
          notifications.push_back({id, disabled});
        }
      }
      else
      {
        log_warning << "Unknown token in config file: " << notification_node.get_name() << std::endl;
      }
    }
  }

  // Compatibility; will be overwritten by the "editor" category
  config_mapping.read("editor_autosave_frequency", editor_autosave_frequency);

  editor_autotile_help = !developer_mode;

  ReaderMapping editor_mapping;
  if (config_mapping.read("editor", editor_mapping))
  {
    editor_mapping.read("autosave_frequency", editor_autosave_frequency);
    editor_mapping.read("autotile_help", editor_autotile_help);
    editor_mapping.read("autotile_mode", editor_autotile_mode);
    editor_mapping.read("render_background", editor_render_background);
    editor_mapping.read("render_grid", editor_render_grid);
    editor_mapping.read("render_lighting", editor_render_lighting);
    editor_mapping.read("selected_snap_grid_size", editor_selected_snap_grid_size);
    editor_mapping.read("snap_to_grid", editor_snap_to_grid);
  }

  if (is_christmas()) {
    christmas_mode = config_mapping.get("christmas", true);
  }
  config_mapping.read("transitions_enabled", transitions_enabled);
  config_mapping.read("locale", locale);
  config_mapping.read("random_seed", random_seed);
  config_mapping.read("repository_url", repository_url);

  config_mapping.read("multiplayer_auto_manage_players", multiplayer_auto_manage_players);
  config_mapping.read("multiplayer_multibind", multiplayer_multibind);
  config_mapping.read("multiplayer_buzz_controllers", multiplayer_buzz_controllers);

  ReaderMapping config_video_mapping;
  if (config_mapping.read("video", config_video_mapping))
  {
    config_video_mapping.read("fullscreen", use_fullscreen);
    std::string video_string;
    config_video_mapping.read("video", video_string);
    video = VideoSystem::get_video_system(video_string);
    config_video_mapping.read("vsync", try_vsync);

    config_video_mapping.read("fullscreen_width",  fullscreen_size.width);
    config_video_mapping.read("fullscreen_height", fullscreen_size.height);
    if (fullscreen_size.width < 0 || fullscreen_size.height < 0)
    {
      // Somehow, an invalid size got entered into the config file,
      // let's use the "auto" setting instead.
      fullscreen_size = Size(0, 0);
    }
    config_video_mapping.read("fullscreen_refresh_rate", fullscreen_refresh_rate);

    config_video_mapping.read("window_width",  window_size.width);
    config_video_mapping.read("window_height", window_size.height);

    config_video_mapping.read("window_resizable", window_resizable);

    config_video_mapping.read("aspect_width",  aspect_size.width);
    config_video_mapping.read("aspect_height", aspect_size.height);

    config_video_mapping.read("magnification", magnification);

#ifdef __EMSCRIPTEN__
    // Forcibly set autofit to true
    // TODO: Remove the autofit parameter entirely - it should always be true

    //config_video_mapping->get("fit_window", fit_window);
    fit_window = true;
#endif
  }

  ReaderMapping config_audio_mapping;
  if (config_mapping.read("audio", config_audio_mapping))
  {
    config_audio_mapping.read("sound_enabled", sound_enabled);
    config_audio_mapping.read("music_enabled", music_enabled);
    config_audio_mapping.read("sound_volume", sound_volume);
    config_audio_mapping.read("music_volume", music_volume);
  }

  ReaderMapping config_control_mapping;
  if (config_mapping.read("control", config_control_mapping))
  {
    ReaderMapping keymap_mapping;
    if (config_control_mapping.read("keymap", keymap_mapping))
    {
      keyboard_config.read(keymap_mapping);
    }

    ReaderMapping joystick_mapping;
    if (config_control_mapping.read("joystick", joystick_mapping))
    {
      joystick_config.read(joystick_mapping);
    }

    mobile_controls = config_control_mapping.get("mobile_controls", SDL_GetNumTouchDevices() > 0);
    m_mobile_controls_scale = config_control_mapping.get("mobile_controls_scale", 1);
  }

  ReaderCollection config_addons_mapping;
  if (config_mapping.read("addons", config_addons_mapping))
  {
    for (auto const& addon_node : config_addons_mapping.get_objects())
    {
      if (addon_node.get_name() == "addon")
      {
        auto addon = addon_node.get_mapping();

        std::string id;
        bool enabled = false;
        if (addon.read("id", id) &&
            addon.read("enabled", enabled))
        {
          addons.push_back({id, enabled});
        }
      }
      else
      {
        log_warning << "Unknown token in config file: " << addon_node.get_name() << std::endl;
      }
    }
  }
}

void
Config::save()
{
  Writer writer("config");

  writer.start_list("supertux-config");

  writer.write("profile", profile);

  writer.start_list("profiles");
  for (auto const& current_profile : profiles)
  {
    writer.start_list("profile");
    writer.write("id", current_profile.id);
    writer.write("name", current_profile.name);
    writer.end_list("profile");
  }
  writer.end_list("profiles");

  writer.write("show_fps", show_fps);
  writer.write("show_player_pos", show_player_pos);
  writer.write("show_controller", show_controller);
  writer.write("developer", developer_mode);
  writer.write("confirmation_dialog", confirmation_dialog);
  writer.write("pause_on_focusloss", pause_on_focusloss);
  writer.write("custom_mouse_cursor", custom_mouse_cursor);

  writer.start_list("notifications");
  for (auto const& notification : notifications)
  {
    writer.start_list("notification");
    writer.write("id", notification.id);
    writer.write("disabled", notification.disabled);
    writer.end_list("notification");
  }
  writer.end_list("notifications");

  writer.write("editor_autosave_frequency", editor_autosave_frequency);

  if (is_christmas()) {
    writer.write("christmas", christmas_mode);
  }
  writer.write("transitions_enabled", transitions_enabled);
  writer.write("locale", locale);
  writer.write("repository_url", repository_url);
  writer.write("multiplayer_auto_manage_players", multiplayer_auto_manage_players);
  writer.write("multiplayer_multibind", multiplayer_multibind);
  writer.write("multiplayer_buzz_controllers", multiplayer_buzz_controllers);

  writer.start_list("interface_colors");
  writer.write("menubackcolor", menubackcolor.toVector());
  writer.write("menufrontcolor", menufrontcolor.toVector());
  writer.write("menuhelpbackcolor", menuhelpbackcolor.toVector());
  writer.write("menuhelpfrontcolor", menuhelpfrontcolor.toVector());
  writer.write("labeltextcolor", labeltextcolor.toVector());
  writer.write("activetextcolor", activetextcolor.toVector());
  writer.write("hlcolor", hlcolor.toVector());
  writer.write("editorcolor", editorcolor.toVector());
  writer.write("editorhovercolor", editorhovercolor.toVector());
  writer.write("editorgrabcolor", editorgrabcolor.toVector());
  writer.write("menuroundness", menuroundness);
  writer.end_list("interface_colors");

  writer.start_list("video");
  writer.write("fullscreen", use_fullscreen);
  if (video == VideoSystem::VIDEO_NULL) {
    // don't save NULL renderer to config as starting SuperTux without
    // getting a window is rather confusing
  } else {
    writer.write("video", VideoSystem::get_video_string(video));
  }
  writer.write("vsync", try_vsync);

  writer.write("fullscreen_width",  fullscreen_size.width);
  writer.write("fullscreen_height", fullscreen_size.height);
  writer.write("fullscreen_refresh_rate", fullscreen_refresh_rate);

  writer.write("window_width",  window_size.width);
  writer.write("window_height", window_size.height);

  writer.write("window_resizable", window_resizable);

  writer.write("aspect_width",  aspect_size.width);
  writer.write("aspect_height", aspect_size.height);

#ifdef __EMSCRIPTEN__
  // Forcibly set autofit to true
  // TODO: Remove the autofit parameter entirely - it should always be true
  writer.write("fit_window", true /* fit_window */);
#endif

  writer.write("magnification", magnification);

  writer.end_list("video");

  writer.start_list("audio");
  writer.write("sound_enabled", sound_enabled);
  writer.write("music_enabled", music_enabled);
  writer.write("sound_volume", sound_volume);
  writer.write("music_volume", music_volume);
  writer.end_list("audio");

  writer.start_list("control");
  {
    writer.start_list("keymap");
    keyboard_config.write(writer);
    writer.end_list("keymap");

    writer.start_list("joystick");
    joystick_config.write(writer);
    writer.end_list("joystick");

    writer.write("mobile_controls", mobile_controls);
    writer.write("mobile_controls_scale", m_mobile_controls_scale);
  }
  writer.end_list("control");

  writer.start_list("addons");
  for (auto const& addon : addons)
  {
    writer.start_list("addon");
    writer.write("id", addon.id);
    writer.write("enabled", addon.enabled);
    writer.end_list("addon");
  }
  writer.end_list("addons");

  writer.start_list("editor");
  {
    writer.write("autosave_frequency", editor_autosave_frequency);
    writer.write("autotile_help", editor_autotile_help);
    writer.write("autotile_mode", editor_autotile_mode);
    writer.write("render_background", editor_render_background);
    writer.write("render_grid", editor_render_grid);
    writer.write("render_lighting", editor_render_lighting);
    writer.write("selected_snap_grid_size", editor_selected_snap_grid_size);
    writer.write("snap_to_grid", editor_snap_to_grid);
  }
  writer.end_list("editor");

  writer.end_list("supertux-config");
}

bool
Config::is_christmas() const
{
  time_t rawtime;
  std::time(&rawtime);
  struct tm* timeinfo = std::localtime(&rawtime);
  // 6. Dec, Saint Nicholas Day
  return timeinfo->tm_mon == 11 && timeinfo->tm_mday >= 6;
}

/* EOF */

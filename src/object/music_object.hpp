//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2018 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_MUSIC_OBJECT_HPP
#define HEADER_SUPERTUX_OBJECT_MUSIC_OBJECT_HPP

#include "supertux/game_object.hpp"

enum MusicType {
  LEVEL_MUSIC,
  HERRING_MUSIC,
  HERRING_WARNING_MUSIC
};

class MusicObject : public GameObject
{
public:
  MusicObject();
  MusicObject(ReaderMapping const& mapping);

  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;

  bool is_singleton() const override { return true; }

  static std::string class_name() { return "music"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Music"); }
  std::string get_display_name() const override { return display_name(); }
  const std::string get_icon_path() const override { return "images/engine/editor/music.png"; }

  void play_music(MusicType musictype);
  void resume_music();
  MusicType get_music_type() const;

  void set_music(std::string const& music);
  std::string get_music() const;

private:
  MusicType m_currentmusic;
  std::string m_music;

private:
  MusicObject(MusicObject const&) = delete;
  MusicObject& operator=(MusicObject const&) = delete;
};

#endif

/* EOF */

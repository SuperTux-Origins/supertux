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

#ifndef HEADER_SUPERTUX_BADGUY_FLAME_HPP
#define HEADER_SUPERTUX_BADGUY_FLAME_HPP

#include "audio/fwd.hpp"
#include "audio/sound_source.hpp"
#include "badguy/badguy.hpp"

class Flame : public BadGuy
{
public:
  Flame(ReaderMapping const& reader,
        std::string const& sprite = "images/creatures/flame/flame.sprite");

  void activate() override;
  void deactivate() override;

  void active_update(float dt_sec) override;
  void kill_fall() override;

  void freeze() override;
  bool is_freezable() const override;
  bool is_flammable() const override;

  static std::string class_name() { return "flame"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Flame"); }
  std::string get_display_name() const override { return display_name(); }

  void stop_looping_sounds() override;
  void play_looping_sounds() override;

  void on_flip(float height) override;

protected:
  float angle;
  float radius;
  float speed;

  std::unique_ptr<SoundSource> sound_source;

private:
  Flame(Flame const&) = delete;
  Flame& operator=(Flame const&) = delete;
};

#endif

/* EOF */

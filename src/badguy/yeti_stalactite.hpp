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

#ifndef HEADER_SUPERTUX_BADGUY_YETI_STALACTITE_HPP
#define HEADER_SUPERTUX_BADGUY_YETI_STALACTITE_HPP

#include "badguy/stalactite.hpp"

class YetiStalactite final : public Stalactite
{
public:
  YetiStalactite(ReaderMapping const& mapping);

  void active_update(float dt_sec) override;
  void draw(DrawingContext& context) override;
  void update(float dt_sec) override;

  bool is_flammable() const override;

  void start_shaking();
  bool is_hanging() const;

private:
  YetiStalactite(YetiStalactite const&) = delete;
  YetiStalactite& operator=(YetiStalactite const&) = delete;
};

#endif

/* EOF */

//  SuperTux
//  Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_STATE_DUMPER_HPP
#define HEADER_SUPERTUX_SUPERTUX_STATE_DUMPER_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>

class Sector;

/** Writes the state of all MovingObjects in a Sector once per logical
    frame. Used with demo playback as a golden-master regression check
    (see tools/golden.sh).

    Format: "frame N", then one "UID TYPE X Y W H [VX VY]" line per
    object whose state changed since the previous frame, and "- UID" for
    objects that went away. */
class StateDumper final
{
public:
  StateDumper(std::string const& filename);

  void dump(Sector& sector);

  /** Record why the run ended (death, finish, demo-end, max-frames). */
  void finish(std::string const& reason);

  int get_frame() const { return m_frame; }

private:
  std::ofstream m_out;
  int m_frame;

  /** last line written per UID, for delta encoding */
  std::unordered_map<uint32_t, std::string> m_last;

private:
  StateDumper(StateDumper const&) = delete;
  StateDumper& operator=(StateDumper const&) = delete;
};

#endif

/* EOF */

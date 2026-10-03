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

#include "supertux/state_dumper.hpp"

#include <algorithm>
#include <format>
#include <vector>
#include <stdexcept>
#include <typeinfo>

#include "object/block.hpp"
#include "object/moving_sprite.hpp"
#include "object/player.hpp"
#include "supertux/moving_object.hpp"
#include "supertux/sector.hpp"

StateDumper::StateDumper(std::string const& filename) :
  m_out(filename),
  m_frame(0)
{
  if (!m_out) {
    throw std::runtime_error("Couldn't open state dump file '" + filename + "' for writing");
  }
  m_out << "# supertux-state 1\n";
}

void
StateDumper::dump(Sector& sector)
{
  m_out << "frame " << m_frame << '\n';

  std::unordered_map<uint32_t, std::string> current;

  for (auto const& object : sector.get_objects())
  {
    if (!object->is_valid())
      continue;

    auto* moving = dynamic_cast<MovingObject*>(object.get());
    if (!moving)
      continue;

    uint32_t const uid = object->get_uid().get_value();
    Rectf const& bbox = moving->get_bbox();
    std::string line = std::format("{} {} {:.3f} {:.3f} {:.3f} {:.3f}",
                                   uid, typeid(*object).name(),
                                   bbox.get_left(), bbox.get_top(),
                                   bbox.get_width(), bbox.get_height());

    if (auto* player = dynamic_cast<Player*>(moving)) {
      Physic const& physic = player->get_physic();
      line += std::format(" {:.3f} {:.3f}", physic.get_velocity_x(), physic.get_velocity_y());
    }

    // collision group and sprite action, compared exactly
    line += std::format(" g:{}", static_cast<int>(moving->get_group()));
    if (auto* sprite = dynamic_cast<MovingSprite*>(moving)) {
      line += " a:" + sprite->get_action();
    } else if (auto* block = dynamic_cast<Block*>(moving)) {
      line += " a:" + block->get_action();
    }

    auto it = m_last.find(uid);
    if (it == m_last.end() || it->second != line) {
      m_out << line << '\n';
    }
    current.emplace(uid, std::move(line));
  }

  // Removals are written sorted so the output does not depend on hash order
  std::vector<uint32_t> removed;
  for (auto const& [uid, line] : m_last) {
    if (!current.contains(uid)) {
      removed.push_back(uid);
    }
  }
  std::sort(removed.begin(), removed.end());
  for (uint32_t uid : removed) {
    m_out << "- " << uid << '\n';
  }

  m_last = std::move(current);
  m_frame += 1;
}

void
StateDumper::finish(std::string const& reason)
{
  m_out << "end " << reason << ' ' << m_frame << '\n';
  m_out.flush();
}

/* EOF */

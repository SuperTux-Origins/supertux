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

#include "supertux/level.hpp"

#include "badguy/goldbomb.hpp"
#include "object/bonus_block.hpp"
#include "object/coin.hpp"
#include "physfs/util.hpp"
#include "supertux/sector.hpp"
#include "trigger/secretarea_trigger.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "util/string_util.hpp"
#include "util/writer.hpp"

#include <physfs.h>
#include <numeric>

Level* Level::s_current = nullptr;

Level::Level(bool worldmap) :
  m_is_worldmap(worldmap),
  m_name("noname"),
  m_author("SuperTux Player"),
  m_contact(),
  m_license(),
  m_filename(),
  m_note(),
  m_sectors(),
  m_stats(),
  m_target_time(),
  m_tileset("images/tiles.strf"),
  m_suppress_pause_menu(),
  m_is_in_cutscene(false),
  m_skip_cutscene(false),
  m_icon(),
  m_icon_locked(),
  m_wmselect_bkg()
{
  s_current = this;
}

Level::~Level()
{
  m_sectors.clear();
}

void
Level::add_sector(std::unique_ptr<Sector> sector)
{
  Sector* test = get_sector(sector->get_name());
  if (test != nullptr) {
    throw std::runtime_error("Trying to add 2 sectors with same name");
  } else {
    m_sectors.push_back(std::move(sector));
  }
}

Sector*
Level::get_sector(const std::string& name_) const
{
  auto _sector = std::find_if(m_sectors.begin(), m_sectors.end(), [name_] (const std::unique_ptr<Sector>& sector) {
    return sector->get_name() == name_;
  });
  if(_sector == m_sectors.end())
    return nullptr;
  return _sector->get();
}

size_t
Level::get_sector_count() const
{
  return m_sectors.size();
}

Sector*
Level::get_sector(size_t num) const
{
  return m_sectors.at(num).get();
}

int
Level::get_total_coins() const
{
  int total_coins = 0;
  for (auto const& sector : m_sectors) {
    for (const auto& o: sector->get_objects()) {
      auto coin = dynamic_cast<Coin*>(o.get());
      if (coin)
      {
        total_coins++;
        continue;
      }
      auto block = dynamic_cast<BonusBlock*>(o.get());
      if (block)
      {
        if (block->get_contents() == BonusBlock::Content::COIN)
        {
          total_coins += block->get_hit_counter();
          continue;
        } else if (block->get_contents() == BonusBlock::Content::RAIN ||
                   block->get_contents() == BonusBlock::Content::EXPLODE)
        {
          total_coins += 10 * block->get_hit_counter();
          continue;
        }
      }
      auto goldbomb = dynamic_cast<GoldBomb*>(o.get());
      if (goldbomb)
        total_coins += 10;
    }
  }
  return total_coins;
}

int
Level::get_total_badguys() const
{
  int total_badguys = 0;
  for (auto const& sector : m_sectors) {
    total_badguys += sector->get_object_count<BadGuy>([] (const BadGuy& badguy) {
      return badguy.m_countMe;
    });
  }
  return total_badguys;
}

int
Level::get_total_secrets() const
{
  auto get_secret_count = [](int accumulator, const std::unique_ptr<Sector>& sector) {
    return accumulator + sector->get_object_count<SecretAreaTrigger>();
  };
  return std::accumulate(m_sectors.begin(), m_sectors.end(), 0, get_secret_count);
}

void
Level::reactivate()
{
  s_current = this;
}

/* EOF */

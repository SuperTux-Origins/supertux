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

#include "badguy/dispenser.hpp"

#include "audio/sound_manager.hpp"
#include "math/random.hpp"
#include "object/bullet.hpp"
#include "object/player.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/sector.hpp"
#include "util/file_system.hpp"
#include "util/reader_mapping.hpp"

const std::vector<std::string> Dispenser::s_sprites = { "cannon.sprite", "dropper.sprite", "invisible.sprite" };

Dispenser::DispenserType
Dispenser::DispenserType_from_string(std::string const& type_string)
{
  if (type_string == "dropper") {
    return DispenserType::DROPPER;
  } else if (type_string == "rocketlauncher") { // Retro-compatibility with "rocketlauncher"
    log_warning("Rocket launcher is no longer available. Replacing with cannon.");
    return DispenserType::CANNON;
  } else if (type_string == "cannon") {
    return DispenserType::CANNON;
  } else if (type_string == "point") {
    return DispenserType::POINT;
  } else {
    throw std::exception();
  }
}

std::string
Dispenser::DispenserType_to_string(DispenserType type)
{
  switch (type)
  {
    case DispenserType::DROPPER:
      return "dropper";
    case DispenserType::CANNON:
      return "cannon";
    case DispenserType::POINT:
      return "point";
    default:
      return "unknown";
  }
}

std::string
Dispenser::Cannon_Direction_to_string(Direction direction)
{
  switch (direction)
  {
    case Direction::LEFT:
      return "left";
    case Direction::RIGHT:
      return "right";
    default:
      if (direction != Direction::AUTO)
        log_warning("Direction \"{}\" not valid for cannon. Switching to \"auto\".", direction);
      return "center";
  }
}

Dispenser::Dispenser(ReaderMapping const& reader) :
  BadGuy(reader, "images/creatures/dispenser/dropper.sprite"),
  ExposedObject<Dispenser, scripting::Dispenser>(this),
  m_cycle(),
  m_badguys(),
  m_next_badguy(0),
  m_dispense_timer(),
  m_autotarget(false),
  m_random(),
  m_gravity(),
  m_type(),
  m_limit_dispensed_badguys(),
  m_max_concurrent_badguys(),
  m_current_badguys()
{
  set_colgroup_active(COLGROUP_MOVING_STATIC);
  SoundManager::current()->preload("sounds/squish.wav");
  m_cycle = reader.get("cycle", 5.0f);
  if (reader.read("gravity", m_gravity)) m_physic.enable_gravity(true);
  if (!reader.read("badguy", m_badguys)) m_badguys.clear();
  m_random = reader.get("random", false);
  std::string type_s = "cannon"; //default
  type_s = reader.get("type", std::string());
  try
  {
    m_type = DispenserType_from_string(type_s);
  }
  catch(std::exception&)
  {
    if (type_s.empty())
    {
      log_warning("No dispenser type set, setting to cannon.");
    }
    else
    {
      log_warning("Unknown type of dispenser:{}, setting to cannon.", type_s);
    }

    m_type = DispenserType::CANNON;
  }

  m_dir = m_start_dir; // Reset direction to default.

  m_limit_dispensed_badguys = reader.get("limit-dispensed-badguys", false);
  m_max_concurrent_badguys = reader.get("max-concurrent-badguys", 0);

  //  if (badguys.size() <= 0)
  //    throw std::runtime_error("No badguys in dispenser.");

  set_correct_action();

  m_col.m_bbox.set_size(m_sprite->get_current_hitbox_width(), m_sprite->get_current_hitbox_height());
  m_countMe = false;
}

  void
  Dispenser::draw(DrawingContext& context)
  {
    if (m_type != DispenserType::POINT)
      BadGuy::draw(context);
  }

void
Dispenser::initialize()
{
  m_dir = m_start_dir; // Reset direction to default.
}

void
Dispenser::activate()
{
  m_dispense_timer.start(m_cycle, true);
  launch_badguy();
}

void
Dispenser::deactivate()
{
  m_dispense_timer.stop();
}

HitResponse
Dispenser::collision(GameObject& other, CollisionHit const& hit)
{
  auto bullet = dynamic_cast<Bullet*> (&other);
  if (bullet)
    return collision_bullet(*bullet, hit);

  return FORCE_MOVE;
}

void
Dispenser::active_update(float dt_sec)
{
  if (m_gravity)
  {
    BadGuy::active_update(dt_sec);
  }
  if (m_dispense_timer.check())
  {
    // auto always shoots in Tux's direction
    if (m_autotarget)
    {
      auto player = get_nearest_player();
      if (player)
      {
        Direction target_dir = (player->get_pos().x > get_pos().x) ? Direction::RIGHT : Direction::LEFT;
        if (m_dir != target_dir)
        {
          m_dir = target_dir;
          return;
        }
      }
    }
    launch_badguy();
  }
}

void
Dispenser::launch_badguy()
{
  if (m_badguys.empty()) return;
  if (m_frozen) return;
  if (m_limit_dispensed_badguys &&
      m_current_badguys >= m_max_concurrent_badguys)
    return;

  //FIXME: Does is_offscreen() work right here?
  if (!is_offscreen())
  {
    Direction launch_dir = m_dir;
    if (!m_autotarget && m_start_dir == Direction::AUTO)
    {
      Player* player = get_nearest_player();
      if (player)
        launch_dir = (player->get_pos().x > get_pos().x) ? Direction::RIGHT : Direction::LEFT;
    }

    if (m_badguys.size() > 1)
    {
      if (m_random)
      {
        m_next_badguy = static_cast<unsigned int>(gameRandom.rand(static_cast<int>(m_badguys.size())));
      }
      else
      {
        m_next_badguy++;

        if (m_next_badguy >= m_badguys.size())
          m_next_badguy = 0;
      }
    }

    std::string badguy = m_badguys[m_next_badguy];

    if (badguy == "random")
    {
      log_warning("random is outdated; use a list of badguys to select from.");
      return;
    }
    if (badguy == "goldbomb")
    {
      log_warning("goldbomb is not allowed to be dispensed");
      return;
    }

    try {
      //Need to allocate the badguy first to figure out its bounding box.
      auto game_object = GameObjectFactory::instance().create(badguy, get_pos(), launch_dir);
      if (game_object == nullptr)
        throw std::runtime_error("Creating " + badguy + " object failed.");

      auto& bad_guy = dynamic_cast<BadGuy&>(*game_object);

      Rectf object_bbox = bad_guy.get_bbox();

      Vector spawnpoint(0.0f, 0.0f);
      switch (m_type)
      {
        case DispenserType::DROPPER:
          if (m_flip == NO_FLIP)
          {
            spawnpoint = get_anchor_pos (m_col.m_bbox, ANCHOR_BOTTOM);
            spawnpoint.x -= 0.5f * object_bbox.get_width();
          }
          else
          {
            spawnpoint = get_anchor_pos (m_col.m_bbox, ANCHOR_TOP);
            spawnpoint.y -= m_col.m_bbox.get_height();
            spawnpoint.x -= 0.5f * object_bbox.get_width();
          }
          break;

        case DispenserType::CANNON:
          spawnpoint = get_pos(); /* top-left corner of the cannon */
          if (launch_dir == Direction::LEFT)
            spawnpoint.x -= object_bbox.get_width() + 1;
          else
            spawnpoint.x += m_col.m_bbox.get_width() + 1;
          if (m_flip != NO_FLIP)
            spawnpoint.y += (m_col.m_bbox.get_height() - 20);
          break;

        case DispenserType::POINT:
          spawnpoint = m_col.m_bbox.p1();
          break;

        default:
          break;
      }

      /* Now we set the real spawn position */
      bad_guy.set_pos(spawnpoint);

      /* We don't want to count dispensed badguys in level stats */
      bad_guy.m_countMe = false;

      /* Set reference to dispenser in badguy itself */
      if (m_limit_dispensed_badguys)
      {
        bad_guy.set_parent_dispenser(this);
        m_current_badguys++;
      }

      Sector::get().add_object(std::move(game_object));
    } catch(std::exception const& e) {
      log_warning("Error dispensing badguy: {}", e.what());
      return;
    }
  }
}

void
Dispenser::freeze()
{
  if (m_type == DispenserType::POINT)
    return;

  set_group(COLGROUP_MOVING_STATIC);
  SoundManager::current()->play("sounds/sizzle.ogg", get_pos());
  m_frozen = true;

  const std::string cannon_iced = "iced-" + Cannon_Direction_to_string(m_dir);
  if (m_type == DispenserType::CANNON && m_sprite->has_action(cannon_iced))
  {
    m_sprite->set_action(cannon_iced, 1);
    // When the dispenser is a cannon, it uses the respective "iced" action, based on the current direction.
  }
  else
  {
    if (m_type == DispenserType::DROPPER && m_sprite->has_action("dropper-iced"))
    {
      m_sprite->set_action("dropper-iced", 1);
      // When the dispenser is a dropper, it uses the "dropper-iced".
    }
    else
    {
      m_sprite->set_color(Color(0.6f, 0.72f, 0.88f));
      m_sprite->stop_animation();
      // When the dispenser is something else (unprobable), or has no matching iced sprite, it shades to blue.
    }
  }
  m_dispense_timer.stop();
}

void
Dispenser::unfreeze(bool melt)
{
  /*set_group(colgroup_active);
    frozen = false;

    sprite->set_color(Color(1.00, 1.00, 1.00f));*/
  BadGuy::unfreeze(melt);

  set_colgroup_active(m_type == DispenserType::POINT ? COLGROUP_DISABLED :
                      COLGROUP_MOVING_STATIC);
  set_correct_action();
  activate();
}

bool
Dispenser::is_freezable() const
{
  return true;
}

bool
Dispenser::is_flammable() const
{
  return false;
}

bool
Dispenser::is_portable() const
{
  return false;
}

void
Dispenser::set_correct_action()
{
  if (std::find(s_sprites.begin(), s_sprites.end(), FileSystem::basename(m_sprite_name)) != s_sprites.end())
    change_sprite("images/creatures/dispenser/" + s_sprites[static_cast<int>(m_type)]);

  switch (m_type)
  {
    case DispenserType::CANNON:
      m_sprite->set_action(Cannon_Direction_to_string(m_dir));
      break;

    case DispenserType::POINT:
      set_colgroup_active(COLGROUP_DISABLED);
      break;

    default:
      break;
  }
}

/* EOF */

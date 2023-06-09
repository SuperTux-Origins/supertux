//  SuperTux - "Will-O-Wisp" Badguy
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
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

#include "badguy/willowisp.hpp"

#include "audio/sound_manager.hpp"
#include "audio/sound_source.hpp"
#include "badguy/dispenser.hpp"
#include "object/lantern.hpp"
#include "object/player.hpp"
#include "supertux/game_session.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"

static const float FLYSPEED = 64.0f; /**< speed in px per second */
static const float TRACK_RANGE = 384.0f; /**< at what distance to start tracking the player */
static const float VANISH_RANGE = 512.0f; /**< at what distance to stop tracking and vanish */
static const std::string SOUNDFILE = "sounds/willowisp.wav";

WillOWisp::WillOWisp(ReaderMapping const& reader) :
  BadGuy(reader, "images/creatures/willowisp/willowisp.sprite", LAYER_FLOATINGOBJECTS,
         "images/objects/lightmap_light/lightmap_light-small.sprite"),
  ExposedObject<WillOWisp, scripting::WillOWisp>(this),
  PathObject(),
  m_mystate(STATE_IDLE),
  m_target_sector(),
  m_target_spawnpoint(),
  m_hit_script(),
  m_sound_source(),
  m_flyspeed(),
  m_track_range(),
  m_vanish_range(),
  m_color(0, 1, 0),
  m_starting_node(0)
{
  m_target_sector = reader.get("sector", std::string("main"));
  m_target_spawnpoint = reader.get("spawnpoint", std::string("main"));
  m_flyspeed = reader.get("flyspeed", FLYSPEED);
  m_track_range = reader.get("track-range", TRACK_RANGE);
  m_vanish_range = reader.get("vanish-range", VANISH_RANGE);
  m_hit_script = reader.get("hit-script", std::string(""));

  bool running;
  if ( !reader.read("running", running)) running = false;

  std::vector<float> color;
  if (reader.read("color", color))
  {
    m_color = Color(color);
  }

  m_starting_node = reader.get("starting-node", 0.f);

  init_path(reader, running);

  m_countMe = false;
  SoundManager::current()->preload(SOUNDFILE);
  SoundManager::current()->preload("sounds/warp.wav");

  m_lightsprite->set_color(Color(m_color.red * 0.2f,
                                 m_color.green * 0.2f,
                                 m_color.blue * 0.2f));
  m_sprite->set_color(m_color);
  m_glowing = true;

  m_sprite->set_action("idle");
}

void
WillOWisp::finish_construction()
{
  if (get_walker() && get_walker()->is_running()) {
    m_mystate = STATE_PATHMOVING_TRACK;
  }
}

void
WillOWisp::active_update(float dt_sec)
{
  auto player = get_nearest_player();
  if (!player) return;
  Vector p1 = m_col.m_bbox.get_middle();
  Vector p2 = player->get_bbox().get_middle();
  Vector dist = (p2 - p1);

  switch (m_mystate) {
    case STATE_STOPPED:
      break;

    case STATE_IDLE:
      if (glm::length(dist) <= m_track_range) {
        m_mystate = STATE_TRACKING;
      }
      break;

    case STATE_TRACKING:
      if (glm::length(dist) > m_vanish_range) {
        vanish();
      } else if (glm::length(dist) >= 1) {
        Vector dir_ = glm::normalize(dist);
        m_col.set_movement(dir_ * dt_sec * m_flyspeed);
      } else {
        /* We somehow landed right on top of the player without colliding.
         * Sit tight and avoid a division by zero. */
      }
      m_sound_source->set_position(get_pos());
      break;

    case STATE_WARPING:
      if (m_sprite->animation_done()) {
        remove_me();
      }
      break;

    case STATE_VANISHING: {
      Vector dir_ = glm::normalize(dist);
      m_col.set_movement(dir_ * dt_sec * m_flyspeed);
      if (m_sprite->animation_done()) {
        remove_me();
      }
      break;
    }

    case STATE_PATHMOVING:
    case STATE_PATHMOVING_TRACK:
      if (get_walker() == nullptr)
        return;
      get_walker()->update(dt_sec);
      m_col.set_movement(get_walker()->get_pos(m_col.m_bbox.get_size(), m_path_handle) - get_pos());
      if (m_mystate == STATE_PATHMOVING_TRACK && glm::length(dist) <= m_track_range) {
        m_mystate = STATE_TRACKING;
      }
      break;

    default:
      assert(false);
  }
}

void
WillOWisp::activate()
{
  m_sound_source = SoundManager::current()->create_sound_source(SOUNDFILE);
  m_sound_source->set_position(get_pos());
  m_sound_source->set_looping(true);
  m_sound_source->set_gain(1.0f);
  m_sound_source->set_reference_distance(32);
  m_sound_source->play();
}

void
WillOWisp::deactivate()
{
  m_sound_source.reset(nullptr);

  switch (m_mystate) {
    case STATE_STOPPED:
    case STATE_IDLE:
    case STATE_PATHMOVING:
    case STATE_PATHMOVING_TRACK:
      break;
    case STATE_TRACKING:
      m_mystate = STATE_IDLE;
      break;
    case STATE_WARPING:
    case STATE_VANISHING:
      remove_me();
      break;
  }
}

void
WillOWisp::vanish()
{
  m_mystate = STATE_VANISHING;
  m_sprite->set_action("vanishing", 1);
  set_colgroup_active(COLGROUP_DISABLED);

  if (m_parent_dispenser != nullptr)
  {
    m_parent_dispenser->notify_dead();
  }
}

bool
WillOWisp::collides(GameObject& other, CollisionHit const& ) const {
  auto lantern = dynamic_cast<Lantern*>(&other);

  //                                 vv  'xor'
  if (lantern && (lantern->is_open() != (get_color().greyscale() == 0)))
    return true;

  if (dynamic_cast<Player*>(&other))
    return true;

  return false;
}

HitResponse
WillOWisp::collision_player(Player& player, CollisionHit const& ) {
  if (player.is_invincible())
    return ABORT_MOVE;

  if (m_mystate != STATE_TRACKING)
    return ABORT_MOVE;

  m_mystate = STATE_WARPING;
  m_sprite->set_action("warping", 1);

  if (!m_hit_script.empty()) {
    Sector::get().run_script(m_hit_script, "hit-script");
  } else {
    GameSession::current()->respawn(m_target_sector, m_target_spawnpoint);
  }
  SoundManager::current()->play("sounds/warp.wav", get_pos());

  return CONTINUE;
}

void
WillOWisp::goto_node(int node_no)
{
  get_walker()->goto_node(node_no);
  if (m_mystate != STATE_PATHMOVING && m_mystate != STATE_PATHMOVING_TRACK) {
    m_mystate = STATE_PATHMOVING;
  }
}

void
WillOWisp::start_moving()
{
  get_walker()->start_moving();
}

void
WillOWisp::stop_moving()
{
  get_walker()->stop_moving();
}

void
WillOWisp::set_state(std::string const& new_state)
{
  if (new_state == "stopped") {
    m_mystate = STATE_STOPPED;
  } else if (new_state == "idle") {
    m_mystate = STATE_IDLE;
  } else if (new_state == "move_path") {
    m_mystate = STATE_PATHMOVING;
    get_walker()->start_moving();
  } else if (new_state == "move_path_track") {
    m_mystate = STATE_PATHMOVING_TRACK;
    get_walker()->start_moving();
  } else if (new_state == "normal") {
    m_mystate = STATE_IDLE;
  } else if (new_state == "vanish") {
    vanish();
  } else {
    log_warning("Can't set unknown willowisp state '{}", new_state);
  }
}

void WillOWisp::stop_looping_sounds()
{
  if (m_sound_source) {
    m_sound_source->stop();
  }
}

void WillOWisp::play_looping_sounds()
{
  if (m_sound_source) {
    m_sound_source->play();
  }
}

void
WillOWisp::move_to(Vector const& pos)
{
  Vector shift = pos - m_col.m_bbox.p1();
  if (get_path()) {
    get_path()->move_by(shift);
  }
  set_pos(pos);
}

/* EOF */

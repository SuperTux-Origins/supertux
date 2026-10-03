//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "object/player.hpp"
#include "object/player_constants.hpp"
#include "ecs/player_systems.hpp"

#include "ecs/registry.hpp"

#include "audio/sound_manager.hpp"
#include "badguy/badguy.hpp"
#include "control/input_manager.hpp"
#include "math/random.hpp"
#include "math/util.hpp"
#include "object/bullet.hpp"
#include "object/camera.hpp"
#include "object/display_effect.hpp"
#include "object/falling_coin.hpp"
#include "object/music_object.hpp"
#include "object/particles.hpp"
#include "object/sprite_particle.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/game_session.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/resources.hpp"
#include "supertux/sector.hpp"
#include "trigger/climbable.hpp"
#include "video/surface.hpp"
#include <sstream>

const float TUX_INVINCIBLE_TIME_WARNING = 2.0f;

using namespace player_constants;

Color
Player::get_player_color(int id)
{
  return Color(1.f - static_cast<float>(id >> 2 & 1) * .4f,
               1.f - static_cast<float>(id >> 1 & 1) * .4f,
               1.f - static_cast<float>(id & 1) * .4f);
}

Player::Player(PlayerStatus& player_status, std::string const& name_, int player_id) :
  ExposedObject<Player, scripting::Player>(this),
  m_id(player_id),
  m_target(nullptr),
  m_deactivated(false),
  m_controller(&InputManager::current()->get_controller(player_id)),
  m_scripting_controller(new CodeController()),
  m_player_status(player_status),
  m_scripting_controller_old(nullptr),
  m_dir(Direction::RIGHT),
  m_old_dir(m_dir),
  m_physic(ecs::emplace<Physic>(get_entity())),
  m_swim(ecs::emplace<PlayerSwim>(get_entity())),
  m_wall(ecs::emplace<PlayerWallJump>(get_entity())),
  m_move(ecs::emplace<PlayerMovement>(get_entity())),
  m_look(ecs::emplace<PlayerAppearance>(get_entity())),
  m_jump(ecs::emplace<PlayerJump>(get_entity())),
  m_life(ecs::emplace<PlayerLife>(get_entity())),
  m_grabbed_object(nullptr),
  m_grabbed_object_remove_listener(new GrabListener(*this)),
  m_released_object(false),
  m_climbing(nullptr),
  m_climbing_remove_listener(nullptr),
  m_ending_direction(0),
  // if/when we have complete penny gfx, we can
  // load those instead of Tux's sprite in the
  // constructor
  m_sprite(SpriteManager::current()->create("images/creatures/tux/tux.sprite")),
  m_lightsprite(SpriteManager::current()->create("images/creatures/tux/light.sprite")),
  m_powersprite(SpriteManager::current()->create("images/creatures/tux/powerups.sprite")),
  m_multiplayer_arrow(SpriteManager::current()->create("images/engine/hud/arrowdown.png")),
  m_airarrow(Surface::from_file("images/engine/hud/airarrow.png")),
  m_tag_fade(nullptr)
{
  m_name = name_;
  m_look.idle_timer.start(static_cast<float>(IDLE_TIME[0]) / 1000.0f);

  SoundManager::current()->preload("sounds/bigjump.wav");
  SoundManager::current()->preload("sounds/brick.wav");
  SoundManager::current()->preload("sounds/jump.wav");
  SoundManager::current()->preload("sounds/hurt.wav");
  SoundManager::current()->preload("sounds/kill.wav");
  SoundManager::current()->preload("sounds/skid.wav");
  SoundManager::current()->preload("sounds/flip.wav");
  SoundManager::current()->preload("sounds/invincible_start.ogg");
  SoundManager::current()->preload("sounds/splash.wav");
  SoundManager::current()->preload("sounds/grow.wav");
  m_col.set_size(TUX_WIDTH, is_big() ? BIG_TUX_HEIGHT : SMALL_TUX_HEIGHT);

  m_sprite->set_angle(0.0f);
  m_powersprite->set_angle(0.0f);
  m_lightsprite->set_angle(0.0f);
  m_lightsprite->set_blend(Blend::ADD);

  m_sprite->set_color(get_player_color(player_id));

  m_physic.reset();
}

Player::~Player()
{
  ungrab_object();
  if (m_climbing) stop_climbing(*m_climbing);
}

float
Player::get_speedlimit() const
{
  return m_move.speedlimit;
}

void
Player::set_speedlimit(float newlimit)
{
  m_move.speedlimit=newlimit;
}

void
Player::set_id(int id)
{
  m_id = id;
  m_controller = &(InputManager::current()->get_controller(id));
}

void
Player::set_controller(Controller const* controller_)
{
  m_controller = controller_;
}

void
Player::set_winning()
{
  if ( ! is_winning() ){
    m_life.winning = true;
    m_life.invincible_timer.start(10000.0f);
  }
}

void
Player::use_scripting_controller(bool use_or_release)
{
  if ((use_or_release == true) && (m_controller != m_scripting_controller.get())) {
    m_scripting_controller_old = &get_controller();
    set_controller(m_scripting_controller.get());
  }
  if ((use_or_release == false) && (m_controller == m_scripting_controller.get())) {
    set_controller(m_scripting_controller_old);
    m_scripting_controller_old = nullptr;
  }
}

void
Player::do_scripting_controller(std::string const& control_text, bool pressed)
{
  if (const auto maybe_control = Control_from_string(control_text)) {
    m_scripting_controller->press(*maybe_control, pressed);
  }
}

bool
Player::adjust_height(float new_height, float bottom_offset)
{
  Rectf bbox2 = m_col.m_bbox;
  bbox2.move(Vector(0, m_col.m_bbox.get_height() - new_height - bottom_offset));
  bbox2.set_height(new_height);

  if (new_height > m_col.m_bbox.get_height()) {
    //Rectf additional_space = bbox2;
    //additional_space.set_height(new_height - m_col.m_bbox.get_height());
    if (!Sector::get().is_free_of_statics(bbox2, this, true))
      return false;
  }

  // adjust bbox accordingly
  // note that we use members of moving_object for this, so we can run this during CD, too
  set_pos(bbox2.p1());
  m_col.set_size(bbox2.get_width(), bbox2.get_height());
  return true;
}

void
Player::trigger_sequence(std::string const& sequence_name, SequenceData const* data)
{
  trigger_sequence(string_to_sequence(sequence_name), data);
}

void
Player::trigger_sequence(Sequence seq, SequenceData const* data)
{
  if (m_climbing) stop_climbing(*m_climbing);
  stop_backflipping();

  GameSession::current()->start_sequence(this, seq, data);
}

void
Player::update(float dt_sec)
{
  PlayerSystems::update_tag(*this, dt_sec);

  // Skip if in multiplayer respawn
  if (is_dead() && m_target && Sector::get().get_object_count<Player>([this](Player const& p) { return !p.is_dead() && !p.is_dying() && !p.is_winning() && &p != this; }))
  {
    auto* target = Sector::get().get_object_by_uid<Player>(*m_target);
    if (!target || target->is_dying() || target->is_dead() || target->is_winning())
    {
      next_target();
    }

    // Respawn input is handled outside PlayerSystems::handle_input(*this) because it happens while the player is dead
    if (is_dead() && m_target)
    {
      if (m_controller->pressed(Control::ACTION))
      {
        multiplayer_respawn();
      }
      else if (m_controller->pressed(Control::LEFT))
      {
        prev_target();
      }
      else if (m_controller->pressed(Control::RIGHT))
      {
        next_target();
      }
    }

    return;
  }

  check_bounds();

  //catch-all for other circumstances in which Tux's hitbox can't be properly adjusted
  if (is_big() &&
    !m_move.duck && !m_swim.swimming && !m_swim.water_jump && !m_jump.backflipping && !m_move.stone &&
    !adjust_height(BIG_TUX_HEIGHT))
  {
    //Force Tux's box up a little in order to not phase into floor
    adjust_height(BIG_TUX_HEIGHT, 10.f);
  }

  if (m_move.velocity_override && glm::length(m_physic.get_velocity()) < SWIM_BOOST_SPEED) {
    m_move.velocity_override = false;
  }

  PlayerSystems::update_swimming(*this);

  if (m_life.dying && m_life.dying_timer.check()) {

    set_bonus(NO_BONUS, true);
    m_life.dead = true;

    if (!Sector::get().get_object_count<Player>([](Player const& p) { return !p.is_dead() && !p.is_dying(); }))
    {
      Sector::get().stop_looping_sounds();
    }
    else
    {
      next_target();
    }
    return;
  }

  if (!m_life.dying && !m_deactivated)
    PlayerSystems::handle_input(*this);

  /*
  // PlayerSystems::handle_input(*this) calls PlayerSystems::apply_friction(*this) when Tux is not walking, so we'll have to do this ourselves
  if (deactivated)
  PlayerSystems::apply_friction(*this);
  */

  PlayerSystems::update_wall_cling(*this);

  PlayerSystems::update_rolling(*this, dt_sec);

  PlayerSystems::update_ground_movement(*this);

  PlayerSystems::update_backflip(*this, dt_sec);

  PlayerSystems::update_landing(*this);

  if (m_look.second_growup_sound_timer.check())
  {
    SoundManager::current()->play("sounds/grow.wav", get_pos());
    m_look.second_growup_sound_timer.stop();
  }

  PlayerSystems::update_boost(*this, dt_sec);

  // calculate movement for this frame
  m_col.set_movement(m_physic.get_movement(dt_sec) + Vector(m_move.boost * dt_sec, 0));

  if (m_grabbed_object != nullptr && !m_life.dying)
  {
    PlayerSystems::position_grabbed_object(*this);
  }

  if (m_life.dying)
    ungrab_object();

  if (!m_move.ice_this_frame && on_ground())
    m_move.on_ice = false;

  m_move.on_ground_flag = false;
  m_move.ice_this_frame = false;

  PlayerSystems::spawn_invincible_sparkles(*this);

  if (m_look.growing) {
    if (m_sprite->animation_done()) m_look.growing = false;
  }

  PlayerSystems::update_climb_animation(*this);
}

bool
Player::on_ground() const
{
  return m_move.on_ground_flag;
}

void
Player::set_on_ground(bool flag)
{
  m_move.on_ground_flag = flag;
}

bool
Player::is_big() const
{
  if (m_player_status.bonus[get_id()] == NO_BONUS)
    return false;

  return true;
}

void
Player::do_cheer()
{
  do_duck();
  do_backflip();
  do_standup(false);
}

void
Player::do_duck() {
  if (m_move.duck)
    return;
  if (!is_big())
    return;

  if (!m_swim.swimming && !m_swim.water_jump && m_physic.get_velocity_y() != 0)
    return;
  if (!on_ground())
    return;
  if (m_jump.does_buttjump)
    return;

  if (adjust_height(DUCKED_TUX_HEIGHT)) {
    m_move.duck = true;
    m_look.growing = false;
    m_move.unduck_hurt_timer.stop();
  } else {
    // FIXME: what now?
  }
}

void
Player::do_standup(bool force_standup) {
  if (!m_move.duck || !is_big() || m_jump.backflipping || m_move.stone)
    return;

  Rectf new_bbox = m_col.m_bbox;
  float new_height = m_swim.swimming ? TUX_WIDTH : BIG_TUX_HEIGHT;
  new_bbox.move(Vector(0, m_col.m_bbox.get_height() - new_height));
  new_bbox.set_height(new_height);
  if (!Sector::get().is_free_of_movingstatics(new_bbox, this) && !force_standup)
    return;

  if (m_swim.swimming ? adjust_height(TUX_WIDTH) : adjust_height(BIG_TUX_HEIGHT)) {
    m_move.duck = false;
    m_move.unduck_hurt_timer.stop();
  } else if (force_standup) {
    // if timer is not already running, start it.
    if (m_move.unduck_hurt_timer.get_period() == 0) {
      m_move.unduck_hurt_timer.start(UNDUCK_HURT_TIME);
    }
    else if (m_move.unduck_hurt_timer.check()) {
      kill(false);
    }
  }

}

void
Player::do_backflip() {
  if (!m_move.duck)
    return;
  if (!on_ground())
    return;

  m_jump.backflip_direction = (m_dir == Direction::LEFT)?(+1):(-1);
  m_jump.backflipping = true;
  do_jump((m_player_status.bonus[get_id()] == AIR_BONUS) ? -720.0f : -580.0f);
  SoundManager::current()->play("sounds/flip.wav", get_pos());
  m_jump.backflip_timer.start(TUX_BACKFLIP_TIME);
}

void
Player::do_jump(float yspeed) {
  if (!m_wall.can_walljump && !m_wall.in_walljump_tile && !on_ground() && !m_jump.coyote_timer.started())
    return;

  // jump only if it would make Tux go faster upwards
  if (m_physic.get_velocity_y() > yspeed) {
    m_physic.set_velocity_y(yspeed);
    //bbox.move(Vector(0, -1));
    m_jump.jumping = true;
    m_move.on_ground_flag = false;
    m_jump.can_jump = false;

    // play sound
    if (is_big()) {
      SoundManager::current()->play("sounds/bigjump.wav", get_pos());
    } else {
      SoundManager::current()->play("sounds/jump.wav", get_pos());
    }
  }
}

void
Player::add_coins(int count)
{
  m_player_status.add_coins(count);
}

int
Player::get_coins() const
{
  return m_player_status.coins;
}

BonusType
Player::string_to_bonus(std::string const& bonus) const {
  BonusType type = NO_BONUS;

  if (bonus == "grow") {
    type = GROWUP_BONUS;
  } else if (bonus == "fireflower") {
    type = FIRE_BONUS;
  } else if (bonus == "iceflower") {
    type = ICE_BONUS;
  } else if (bonus == "airflower") {
    type = AIR_BONUS;
  } else if (bonus == "earthflower") {
    type = EARTH_BONUS;
  } else if (bonus == "none") {
    type = NO_BONUS;
  } else {
    std::ostringstream msg;
    msg << "Unknown bonus type "  << bonus;
    throw std::runtime_error(msg.str());
  }

  return type;
}

bool
Player::add_bonus(std::string const& bonustype)
{
  return add_bonus( string_to_bonus(bonustype) );
}

bool
Player::set_bonus(std::string const& bonustype)
{
  return set_bonus( string_to_bonus(bonustype) );
}

bool
Player::add_bonus(BonusType type, bool animate)
{
  // always ignore NO_BONUS
  if (type == NO_BONUS) {
    return true;
  }

  // ignore GROWUP_BONUS if we're already big
  if (type == GROWUP_BONUS) {
    if (m_player_status.bonus[get_id()] != NO_BONUS)
      return true;
  }

  return set_bonus(type, animate);
}

bool
Player::set_bonus(BonusType type, bool animate)
{
  if (m_life.dying) {
    return false;
  }

  if ((m_player_status.bonus[get_id()] == NO_BONUS) && (type != NO_BONUS || m_move.stone)) {
    if (!m_swim.swimming)
    {
      if (!adjust_height(BIG_TUX_HEIGHT))
      {
        log_debug("Can't adjust Tux height");
        return false;
      }
    }
    if (animate) {
      m_look.growing = true;
      if (m_climbing)
        m_sprite->set_action("grow-ladder", m_dir, 1);
      else
        m_sprite->set_action("grow", m_dir , 1);
    }
    if (m_climbing) stop_climbing(*m_climbing);
  }

  if (type == NO_BONUS) {
    if (!adjust_height(SMALL_TUX_HEIGHT)) {
      log_debug("Can't adjust Tux height");
      return false;
    }
    if (m_jump.does_buttjump) m_jump.does_buttjump = false;
  }

  if ((type == NO_BONUS) || (type == GROWUP_BONUS)) {
    Vector ppos = Vector((m_col.m_bbox.get_left() + m_col.m_bbox.get_right()) / 2, m_col.m_bbox.get_top());
    Vector pspeed = Vector(((m_dir == Direction::LEFT) ? 100.0f : -100.0f), -300.0f);
    Vector paccel = Vector(0, 1000);
    std::string action = (m_dir == Direction::LEFT) ? "left" : "right";
    std::string particle_name = "";

    if ((m_player_status.bonus[get_id()] == FIRE_BONUS) && (animate)) {
      // visually lose helmet
      if (g_config->christmas_mode) {
        particle_name = "santatux-hat";
      }
      else {
        particle_name = "firetux-helmet";
      }
    }
    if ((m_player_status.bonus[get_id()] == ICE_BONUS) && (animate)) {
      // visually lose cap
      particle_name = "icetux-cap";
    }
    if ((m_player_status.bonus[get_id()] == AIR_BONUS) && (animate)) {
      // visually lose hat
      particle_name = "airtux-hat";
    }
    if ((m_player_status.bonus[get_id()] == EARTH_BONUS) && (animate)) {
      // visually lose hard-hat
      particle_name = "earthtux-hardhat";
    }
    if (!particle_name.empty() && animate) {
      Sector::get().add<SpriteParticle>("images/particles/" + particle_name + ".sprite",
                                             action, ppos, ANCHOR_TOP, pspeed, paccel, LAYER_OBJECTS - 1, true);
    }

    m_player_status.max_fire_bullets[get_id()] = 0;
    m_player_status.max_ice_bullets[get_id()] = 0;
    m_player_status.max_air_time[get_id()] = 0;
    m_player_status.max_earth_time[get_id()] = 0;
  }
  if (type == FIRE_BONUS) m_player_status.max_fire_bullets[get_id()]++;
  if (type == ICE_BONUS) m_player_status.max_ice_bullets[get_id()]++;
  if (type == AIR_BONUS) m_player_status.max_air_time[get_id()]++;
  if (type == EARTH_BONUS) m_player_status.max_earth_time[get_id()]++;

  if (!m_look.second_growup_sound_timer.started() &&
     type > GROWUP_BONUS && type != m_player_status.bonus[get_id()])
  {
    m_look.second_growup_sound_timer.start(0.5);
  }

  m_player_status.bonus[get_id()] = type;
  return true;
}

void
Player::set_visible(bool visible_)
{
  m_look.visible = visible_;
}

bool
Player::get_visible() const
{
  return m_look.visible;
}

void
Player::kick()
{
  m_move.kick_timer.start(KICK_TIME);
}

void
Player::draw(DrawingContext& context)
{
  PlayerSystems::draw(*this, context);
}

void
Player::collision_tile(uint32_t tile_attributes)
{
  if (tile_attributes & Tile::HURTS)
  {
    if (tile_attributes & Tile::UNISOLID)
      kill(false);
    else
    {
      Rectf hurtbox = get_bbox().grown(-6.f);
      if (!Sector::get().is_free_of_tiles(hurtbox, false, Tile::HURTS))
        kill(false);
    }
  }

  if (tile_attributes & Tile::WALLJUMP)
  {
    m_wall.in_walljump_tile = true;
  }

  if (tile_attributes & Tile::ICE) {
    m_move.ice_this_frame = true;
    m_move.on_ice = true;
  }
}

void
Player::collision_solid(CollisionHit const& hit)
{
  if (hit.bottom) {
    if (m_physic.get_velocity_y() > 0)
      m_physic.set_velocity_y(0);

    if (!m_swim.swimming)
      m_move.on_ground_flag = true;
    m_move.floor_normal = hit.slope_normal;

    // Butt Jump landed
    if (m_jump.does_buttjump) {
      m_jump.does_buttjump = false;
      m_physic.set_velocity_y(-300);
      m_move.on_ground_flag = false;
      Sector::get().add<Particles>(
        m_col.m_bbox.p2(),
        50, 70, 260.0f, 280.0f, Vector(0, 300), 3,
        Color(.4f, .4f, .4f), 3, .8f, LAYER_OBJECTS+1);
      Sector::get().add<Particles>(
        Vector(m_col.m_bbox.get_left(), m_col.m_bbox.get_bottom()),
        -70, -50, 260.0f, 280.0f, Vector(0, 300), 3,
        Color(.4f, .4f, .4f), 3, .8f, LAYER_OBJECTS+1);
      Sector::get().get_camera().shake(.1f, 0, 5);
    }

  } else if (hit.top) {
    if (m_physic.get_velocity_y() < 0)
      m_physic.set_velocity_y(.2f);
  }

  if (m_move.stone && m_move.floor_normal.y == 0 && (((m_physic.get_velocity_x() < -MAX_RUN_XM) && hit.left) ||
    ((m_physic.get_velocity_x() > MAX_RUN_XM) && hit.right)))
  {
    m_physic.set_acceleration_x(0);
    m_physic.set_velocity_x(0);
    stop_rolling();
  }

  if ((hit.left || hit.right) && hit.slope_normal.x == 0) {
    m_physic.set_velocity_x(0);
  }

  // crushed?
  if (hit.crush) {
    if (hit.left || hit.right) {
      kill(true);
    } else if (hit.top || hit.bottom) {
      kill(false);
    }
  }

  if ((hit.left && m_move.boost < 0.f) || (hit.right && m_move.boost > 0.f))
    m_move.boost = 0.f;
}

HitResponse
Player::collision(GameObject& other, CollisionHit const& hit)
{
  auto bullet = dynamic_cast<Bullet*> (&other);
  if (bullet) {
    return FORCE_MOVE;
  }

  auto player = dynamic_cast<Player*> (&other);
  if (player) {
    return ABORT_MOVE;
  }

  if (hit.left || hit.right) {
    PlayerSystems::try_grab(*this); //grab objects right now, in update it will be too late
  }
  assert(dynamic_cast<MovingObject*> (&other) != nullptr);
  auto moving_object = static_cast<MovingObject*> (&other);
  if (moving_object->get_group() == COLGROUP_TOUCHABLE) {
    auto trigger = dynamic_cast<TriggerBase*> (&other);
    if (trigger && !m_deactivated) {
      if (m_controller->pressed(Control::UP))
        trigger->event(*this, TriggerBase::EVENT_ACTIVATE);
    }

    return FORCE_MOVE;
  }

  auto badguy = dynamic_cast<BadGuy*> (&other);
  if (badguy != nullptr) {
    if (m_life.safe_timer.started() || m_life.invincible_timer.started())
      return FORCE_MOVE;
    if (m_move.stone)
      return ABORT_MOVE;
  }

  return CONTINUE;
}

void
Player::remove_me()
{
  InputManager::current()->on_player_removed(get_id());
  MovingObject::remove_me();
}

void
Player::make_invincible()
{
  // No get_pos() here since the music affects the whole sector
  SoundManager::current()->play("sounds/invincible_start.ogg");
  m_life.invincible_timer.start(TUX_INVINCIBLE_TIME);
  Sector::get().get_singleton_by_type<MusicObject>().play_music(HERRING_MUSIC);
}

void
Player::kill(bool completely)
{
  if (m_life.dying || m_deactivated || is_winning() )
    return;

  if (!completely && (m_life.safe_timer.started() || m_life.invincible_timer.started()))
    return;

  m_look.growing = false;

  if (m_climbing) stop_climbing(*m_climbing);

  m_physic.set_velocity_x(0);
  m_move.boost = 0.f;

  m_sprite->set_angle(0.0f);
  m_powersprite->set_angle(0.0f);
  m_lightsprite->set_angle(0.0f);

  if (!completely && is_big()) {
    SoundManager::current()->play("sounds/hurt.wav", get_pos());

    if (m_player_status.bonus[get_id()] == FIRE_BONUS
      || m_player_status.bonus[get_id()] == ICE_BONUS
      || m_player_status.bonus[get_id()] == AIR_BONUS
      || m_player_status.bonus[get_id()] == EARTH_BONUS) {
      m_life.safe_timer.start(TUX_SAFE_TIME);
      set_bonus(GROWUP_BONUS, true);
    } else if (m_player_status.bonus[get_id()] == GROWUP_BONUS) {
      m_life.safe_timer.start(TUX_SAFE_TIME /* + GROWING_TIME */);
      m_move.duck = false;
      stop_backflipping();
      set_bonus(NO_BONUS, true);
    }
  } else {
    SoundManager::current()->play("sounds/kill.wav", get_pos());

    // do not die when in edit mode
    if (m_life.edit_mode) {
      set_ghost_mode(true);
      return;
    }

    m_physic.enable_gravity(true);
    m_physic.set_gravity_modifier(1.0f); // Undo jump_early_apex
    m_life.safe_timer.stop();
    m_life.invincible_timer.stop();
    m_physic.set_acceleration(0, 0);
    m_physic.set_velocity(0, -700);
    set_bonus(NO_BONUS, true);
    m_life.dying = true;
    m_life.dying_timer.start(3.0);
    set_group(COLGROUP_DISABLED);

    auto alive_players = Sector::get().get_object_count<Player>([](Player const& p){ return !p.is_dead() && !p.is_dying(); });

    if (!alive_players)
    {
      if (m_player_status.can_reach_checkpoint())
      {
        for (int i = 0; i < 5; i++)
        {
          // the numbers: starting x, starting y, velocity y
          Sector::get().add<FallingCoin>(get_pos() +
                                                        Vector(graphicsRandom.randf(5.0f), graphicsRandom.randf(-32.0f, 18.0f)),
                                                        graphicsRandom.randf(-100.0f, 100.0f));
        }
        m_player_status.take_checkpoint_coins();
      }
      else
      {
        GameSession::current()->set_reset_point("", Vector(0.0f, 0.0f));
      }

      Sector::get().get_effect().fade_out(3.0);
      SoundManager::current()->pause_music(3.0);
    }
  }
}

void
Player::move(Vector const& vector)
{
  set_pos(vector);

  // Reset size to get correct hitbox if Tux was eg. ducked before moving
  if (is_big())
    m_col.set_size(TUX_WIDTH, BIG_TUX_HEIGHT);
  else
    m_col.set_size(TUX_WIDTH, SMALL_TUX_HEIGHT);
  m_move.duck = false;
  stop_backflipping();
  m_jump.last_ground_y = vector.y;
  if (m_climbing) stop_climbing(*m_climbing);

  m_physic.reset();
}

void
Player::check_bounds()
{
  /* Keep tux in sector bounds: */
  if (get_pos().x < 0) {
    // Lock Tux to the size of the level, so that he doesn't fall off
    // the left side
    set_pos(Vector(0, get_pos().y));
  }

  if (m_col.m_bbox.get_right() > Sector::get().get_width()) {
    // Lock Tux to the size of the level, so that he doesn't fall off
    // the right side
    set_pos(Vector(Sector::get().get_width() - m_col.m_bbox.get_width(),
                   m_col.m_bbox.get_top()));
  }

  // If Tux is swimming, don't allow him to go below the sector
  if (m_swim.swimming && !m_life.ghost_mode && !is_dying() && !is_dead()
      && m_col.m_bbox.get_bottom() > Sector::get().get_height()) {
    set_pos(Vector(m_col.m_bbox.get_left(),
                   Sector::get().get_height() - m_col.m_bbox.get_height()));
  }

  /* fallen out of the level? */
  if ((get_pos().y > Sector::get().get_height()) && (!m_life.ghost_mode)) {
    kill(true);
    return;
  }
}

void
Player::add_velocity(Vector const& velocity)
{
  m_physic.set_velocity(m_physic.get_velocity() + velocity);
}

void
Player::add_velocity(Vector const& velocity, Vector const& end_speed)
{
  if (end_speed.x > 0)
    m_physic.set_velocity_x(std::min(m_physic.get_velocity_x() + velocity.x, end_speed.x));
  if (end_speed.x < 0)
    m_physic.set_velocity_x(std::max(m_physic.get_velocity_x() + velocity.x, end_speed.x));
  if (end_speed.y > 0)
    m_physic.set_velocity_y(std::min(m_physic.get_velocity_y() + velocity.y, end_speed.y));
  if (end_speed.y < 0)
    m_physic.set_velocity_y(std::max(m_physic.get_velocity_y() + velocity.y, end_speed.y));
}

Vector
Player::get_velocity() const
{
  return m_physic.get_velocity();
}

void
Player::bounce(BadGuy& )
{
  if (!(m_player_status.bonus[get_id()] == AIR_BONUS))
    m_physic.set_velocity_y(m_controller->hold(Control::JUMP) ? -520.0f : -300.0f);
  else {
    m_physic.set_velocity_y(m_controller->hold(Control::JUMP) ? -580.0f : -340.0f);
  }
}

//scripting Functions Below

void
Player::deactivate()
{
  if (m_deactivated)
    return;
  m_deactivated = true;
  m_physic.set_velocity_x(0);
  m_physic.set_velocity_y(0);
  m_physic.set_acceleration_x(0);
  m_physic.set_acceleration_y(0);
  if (m_climbing) stop_climbing(*m_climbing);
}

void
Player::activate()
{
  if (!m_deactivated)
    return;
  m_deactivated = false;
}

void Player::walk(float speed)
{
  m_physic.set_velocity_x(speed);
}

void Player::set_dir(bool right)
{
  m_dir = right ? Direction::RIGHT : Direction::LEFT;
}

void
Player::set_ghost_mode(bool enable)
{
  if (m_life.ghost_mode == enable)
    return;

  if (m_climbing) stop_climbing(*m_climbing);

  ungrab_object();

  if (enable) {
    m_life.ghost_mode = true;
    set_group(COLGROUP_DISABLED);
    m_physic.enable_gravity(false);
    log_debug("You feel lightheaded. Use movement controls to float around, press ACTION to scare badguys.");
  } else {
    m_life.ghost_mode = false;
    set_group(COLGROUP_MOVING);
    m_physic.enable_gravity(true);
    log_debug("You feel solid again.");
  }
}

void
Player::set_edit_mode(bool enable)
{
  m_life.edit_mode = enable;
}

void
Player::start_climbing(Climbable& climbable)
{
  if (m_climbing || m_swim.swimming)
    return;

  m_climbing = &climbable;
  m_sprite->set_angle(0.0f);
  m_move.boost = 0.f;
  m_physic.enable_gravity(false);
  m_physic.set_velocity(0, 0);
  m_physic.set_acceleration(0, 0);
  if (m_jump.backflipping) {
    stop_backflipping();
    do_standup(true);
  }
}

void
Player::stop_climbing(Climbable& /*climbable*/)
{
  if (!m_climbing) return;

  m_climbing = nullptr;

  ungrab_object();

  m_physic.enable_gravity(true);
  m_physic.set_velocity(0, 0);
  m_physic.set_acceleration(0, 0);

  if (m_controller->hold(Control::JUMP)) {
    m_move.on_ground_flag = true;
    m_jump.early_apex = false;
    do_jump(m_player_status.bonus[get_id()] == BonusType::AIR_BONUS ? -540.0f : -480.0f);
  }
  else if (m_controller->hold(Control::UP)) {
    m_move.on_ground_flag = true;
    // TODO: This won't help. Why?
    do_jump(-300);
  }
}

void
Player::stop_backflipping()
{
  m_jump.backflipping = false;
  m_jump.backflip_direction = 0;
  m_sprite->set_angle(0.0f);
  m_powersprite->set_angle(0.0f);
  m_lightsprite->set_angle(0.0f);
}

bool
Player::has_grabbed(std::string const& object_name) const
{
  if (object_name.empty())
  {
    return false;
  }
  if (auto object = dynamic_cast<GameObject*>(m_grabbed_object))
  {
    return object->get_name() == object_name;
  }
  return false;
}

void
Player::sideways_push(float delta)
{
  m_move.boost = delta;
}

void
Player::ungrab_object(GameObject* gameobject)
{
  if (!m_grabbed_object)
    return;

  // If gameobject is not null, then the function was called from the
  // ObjectRemoveListener.
  if (!gameobject)
    m_grabbed_object->ungrab(*this, m_dir);

  GameObject* go = dynamic_cast<GameObject*>(m_grabbed_object);

  if (go && m_grabbed_object_remove_listener)
    go->del_remove_listener(m_grabbed_object_remove_listener.get());

  m_grabbed_object = nullptr;
}

void
Player::next_target()
{
  auto const& players = Sector::get().get_players();

  Player* first = nullptr;
  bool is_next = false;
  for (auto* player : players)
  {
    if (!player->is_dead() && !player->is_dying() && !player->is_winning())
    {
      if (!first)
      {
        first = player;
      }

      if (is_next)
      {
        m_target.reset(new UID());
        *m_target = player->get_uid();
        return;
      }

      if (m_target && player->get_uid() == *m_target)
      {
        is_next = true;
      }
    }
  }

  if (first)
  {
    m_target.reset(new UID());
    *m_target = first->get_uid();
  }
  else
  {
    m_target.reset(nullptr);
  }
}

void
Player::prev_target()
{
  auto const& players = Sector::get().get_players();

  Player* last = nullptr;
  for (auto* player : players)
  {
    if (!player->is_dead() && !player->is_dying() && !player->is_winning())
    {
      if (m_target && player->get_uid() == *m_target && last)
      {
        *m_target = last->get_uid();
        return;
      }

      last = player;
    }
  }

  if (last)
  {
    m_target.reset(new UID());
    *m_target = last->get_uid();
  }
  else
  {
    m_target.reset(nullptr);
  }
}

void
Player::multiplayer_prepare_spawn()
{
  m_physic.enable_gravity(true);
  m_physic.set_gravity_modifier(1.0f); // Undo jump_early_apex
  m_life.safe_timer.stop();
  m_life.invincible_timer.stop();
  m_physic.set_acceleration(0, -9999);
  m_physic.set_velocity(0, -9999);
  m_life.dying = true;
  set_group(COLGROUP_DISABLED);
  m_life.dead = true;

  next_target();
}

void
Player::multiplayer_respawn()
{
  if (!m_target)
  {
    log_warning("Can't respawn multiplayer player, no target");
    return;
  }

  auto target = Sector::get().get_object_by_uid<Player>(*m_target);

  if (!target)
  {
    log_warning("Can't respawn multiplayer player, target missing");
    return;
  }

  m_life.dying = false;
  m_life.dead = false;
  m_deactivated = false;
  m_life.ghost_mode = false;
  set_group(COLGROUP_MOVING);
  m_physic.reset();

  move(target->get_pos());
  m_target.reset();
}

void
Player::stop_rolling(bool violent)
{
  m_sprite->set_angle(0.0f);
  if (!m_swim.swimming && !m_swim.water_jump && !m_move.duck)
  {
    if (!adjust_height(BIG_TUX_HEIGHT))
    {
      adjust_height(BIG_TUX_HEIGHT, 10.f);
      do_duck();
    }
  }
  if (violent)
  {
    for (int i = 0; i < 5; i++)
    {
      Vector pspeed = Vector(graphicsRandom.randf(-100.f, 100.f)*(static_cast<float>(i)-2), graphicsRandom.randf(-200.f, -150.f));
      Vector paccel = Vector(0, 1000.f + graphicsRandom.randf(-100.f, 100.f));
      Sector::get().add<SpriteParticle>(
        "images/particles/rock.sprite", "rock-"+std::to_string(i),
        get_bbox().get_middle(),
        ANCHOR_MIDDLE, pspeed, paccel, LAYER_OBJECTS + 6, true);
    }
    SoundManager::current()->play("sounds/brick.wav", get_pos());
  }
  m_move.stone = false;
}

/* EOF */

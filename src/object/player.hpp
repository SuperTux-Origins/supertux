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

#ifndef HEADER_SUPERTUX_OBJECT_PLAYER_HPP
#define HEADER_SUPERTUX_OBJECT_PLAYER_HPP

#include "scripting/player.hpp"
#include "sprite/sprite_ptr.hpp"
#include "ecs/player_components.hpp"
#include "squirrel/exposed_object.hpp"
#include "supertux/direction.hpp"
#include "supertux/moving_object.hpp"
#include "supertux/object_remove_listener.hpp"
#include "supertux/physic.hpp"
#include "supertux/player_status.hpp"
#include "supertux/sequence.hpp"
#include "supertux/timer.hpp"
#include "video/color.hpp"
#include "video/layer.hpp"
#include "video/surface_ptr.hpp"

class BadGuy;
class Portable;
class Climbable;
class Controller;
class CodeController;

extern const float TUX_INVINCIBLE_TIME_WARNING;

class Player final : public MovingObject,
                     public ExposedObject<Player, scripting::Player>
{
public:
  using FallMode = PlayerJump::FallMode;
  static constexpr FallMode ON_GROUND = PlayerJump::ON_GROUND;
  static constexpr FallMode JUMPING = PlayerJump::JUMPING;
  static constexpr FallMode TRAMPOLINE_JUMP = PlayerJump::TRAMPOLINE_JUMP;
  static constexpr FallMode FALLING = PlayerJump::FALLING;

private:
  class GrabListener final : public ObjectRemoveListener
  {
  public:
    GrabListener(Player& player) : m_player(player)
    {}

    void object_removed(GameObject* object) override {
      m_player.ungrab_object(object);
    }

  private:
    Player& m_player;

  private:
    GrabListener(GrabListener const&) = delete;
    GrabListener& operator=(GrabListener const&) = delete;
  };

public:
  static Color get_player_color(int id);

public:
  Player(PlayerStatus& player_status, std::string const& name, int player_id);
  ~Player() override;

  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void collision_tile(uint32_t tile_attributes) override;
  bool is_singleton() const override { return false; }
  void remove_me() override;

  int get_id() const { return m_id; }
  void set_id(int id);

  int get_layer() const override { return LAYER_OBJECTS + 1; }

  void set_controller(Controller const* controller);
  /** Level solved. Don't kill Tux any more. */
  void set_winning();
  bool is_winning() const { return m_life.winning; }

  // Tux can only go this fast. If set to 0 no special limit is used, only the default limits.
  void set_speedlimit(float newlimit);
  float get_speedlimit() const;

  Controller const& get_controller() const { return *m_controller; }

  void use_scripting_controller(bool use_or_release);
  void do_scripting_controller(std::string const& control, bool pressed);

  void make_invincible();

  bool is_invincible() const { return m_life.invincible_timer.started(); }
  bool is_dying() const { return m_life.dying; }

  Direction peeking_direction_x() const { return m_move.peeking_x; }
  Direction peeking_direction_y() const { return m_move.peeking_y; }

  void kill(bool completely);
  void move(Vector const& vector);

  bool add_bonus(std::string const& bonus);
  bool set_bonus(std::string const& bonus);
  void add_coins(int count);
  int get_coins() const;

  /** picks up a bonus, taking care not to pick up lesser bonus items than we already have

      @returns true if the bonus has been set (or was already good enough)
               false if the bonus could not be set (for example no space for big tux) */
  bool add_bonus(BonusType type, bool animate = false);

  /** like add_bonus, but can also downgrade the bonus items carried */
  bool set_bonus(BonusType type, bool animate = false);

  PlayerStatus& get_status() const { return m_player_status; }

  /** set kick animation */
  void kick();

  /** play cheer animation.
      This might need some space and behave in an unpredictable way.
      Best to use this at level end. */
  void do_cheer();

  /** duck down if possible.
      this won't last long as long as input is enabled. */
  void do_duck();

  /** stand back up if possible. */
  void do_standup(bool force_standup);

  /** do a backflip if possible. */
  void do_backflip();

  /** jump in the air if possible
      sensible values for yspeed are negative - unless we want to jump
      into the ground of course */
  void do_jump(float yspeed);

  /** Adds velocity to the player (be careful when using this) */
  void add_velocity(Vector const& velocity);

  /** Adds velocity to the player until given end speed is reached */
  void add_velocity(Vector const& velocity, Vector const& end_speed);

  /** Returns the current velocity of the player */
  Vector get_velocity() const;

  void bounce(BadGuy& badguy);
  void override_velocity() { m_move.velocity_override = true; }

  bool is_dead() const { return m_life.dead; }
  bool is_big() const;
  bool is_stone() const { return m_move.stone; }
  bool is_swimming() const { return m_swim.swimming; }
  bool is_swimboosting() const { return m_swim.boosting; }
  bool is_water_jumping() const { return m_swim.water_jump; }
  bool is_skidding() const { return m_move.skidding_timer.started(); }
  float get_swimming_angle() const { return m_swim.angle; }

  void set_visible(bool visible);
  bool get_visible() const;

  bool on_ground() const;
  void set_on_ground(bool flag);

  Portable* get_grabbed_object() const { return m_grabbed_object; }
  void stop_grabbing() { m_grabbed_object = nullptr; }

  /** Checks whether the player has grabbed a certain object
      @param name Name of the object to check */
  bool has_grabbed(std::string const& object_name) const;

  /** Switches ghost mode on/off.
      Lets Tux float around and through solid objects. */
  void set_ghost_mode(bool enable);

  /** Switches edit mode on/off.
      In edit mode, Tux will enter ghost_mode instead of dying. */
  void set_edit_mode(bool enable);

  /** Returns whether ghost mode is currently enabled */
  bool get_ghost_mode() const { return m_life.ghost_mode; }

  /** Changes height of bounding box.
      Returns true if successful, false otherwise */
  bool adjust_height(float new_height, float bottom_offset = 0);

  /** Orders the current GameSession to start a sequence
      @param sequence_name Name of the sequence to start
      @param data Custom additional sequence data */
  void trigger_sequence(std::string const& sequence_name, SequenceData const* data = nullptr);

  /** Orders the current GameSession to start a sequence
      @param sequence Sequence to start
      @param data Custom additional sequence data */
  void trigger_sequence(Sequence seq, SequenceData const* data = nullptr);

  /** Requests that the player start climbing the given Climbable */
  void start_climbing(Climbable& climbable);

  /** Requests that the player stop climbing the given Climbable */
  void stop_climbing(Climbable& climbable);

  Physic& get_physic() { return m_physic; }

  void activate();
  void deactivate();

  void walk(float speed);
  void set_dir(bool right);
  void stop_backflipping();

  void position_grabbed_object();
  bool try_grab();

  /** Boosts Tux in a certain direction, sideways. Useful for bumpers/walljumping. */
  void sideways_push(float delta);

  void multiplayer_prepare_spawn();

  void set_ending_direction(int direction) { m_ending_direction = direction; }
  int get_ending_direction() const { return m_ending_direction; }

private:
  void handle_input();
  void handle_input_ghost(); /**< input handling while in ghost mode */
  void handle_input_climbing(); /**< input handling while climbing */
  void handle_input_rolling();

  void handle_input_swimming();

  void handle_horizontal_input();
  void handle_vertical_input();

  void do_jump_apex();
  void early_jump_apex();

  void swim(float pointx, float pointy, bool boost);

  BonusType string_to_bonus(std::string const& bonus) const;

  /** slows Tux down a little, based on where he's standing */
  void apply_friction();

  void check_bounds();

  /**
   * Ungrabs the currently grabbed object, if any. Only call with its argument
   * from an ObjectRemoveListener.
   */
  void ungrab_object(GameObject* gameobject = nullptr);

  void next_target();
  void prev_target();

  void multiplayer_respawn();

  void stop_rolling(bool violent = true);

private:
  int m_id;
  std::unique_ptr<UID> m_target; /**< (Multiplayer) If not null, then the player does not exist in game and is offering the player to spawn at that player's position */
  bool m_deactivated;

  Controller const* m_controller;
  std::unique_ptr<CodeController> m_scripting_controller; /**< This controller is used when the Player is controlled via scripting */
  PlayerStatus& m_player_status;
  Controller const* m_scripting_controller_old; /**< Saves the old controller while the scripting_controller is used */

public:
  Direction m_dir;

private:
  Direction m_old_dir;

  // State kept in the registry (ecs/player_components.hpp)
  Physic& m_physic;
  PlayerSwim& m_swim;
  PlayerWallJump& m_wall;
  PlayerMovement& m_move;
  PlayerAppearance& m_look;

public:
  /** public for badguys, objects and the camera */
  PlayerJump& m_jump;
  PlayerLife& m_life;

private:
  Portable* m_grabbed_object;
  std::unique_ptr<ObjectRemoveListener> m_grabbed_object_remove_listener;
  bool m_released_object;

  Climbable* m_climbing; /**< Climbable object we are currently climbing, null if none */
  std::unique_ptr<ObjectRemoveListener> m_climbing_remove_listener;

  int m_ending_direction;

  SpritePtr m_sprite; /**< The main sprite representing Tux */
  SpritePtr m_lightsprite;
  SpritePtr m_powersprite;
  SpritePtr m_multiplayer_arrow;
  SurfacePtr m_airarrow; /**< arrow indicating Tux' position when he's above the camera */

  // Multiplayer tag stuff (number displayed over the players)
  std::unique_ptr<FadeHelper> m_tag_fade;

private:
  friend struct PlayerSystems;

  Player(Player const&) = delete;
  Player& operator=(Player const&) = delete;
};

#endif

/* EOF */

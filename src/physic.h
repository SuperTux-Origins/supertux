// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_PHYSIC_H
#define SUPERTUX_PHYSIC_H

/**
 * Horizontal/vertical velocity and acceleration with optional gravity.
 * Note: set/get_velocity_y and set/get_acceleration_y invert the sign so
 * positive values mean "up" at the call site while storage is screen-down.
 */
class Physic
{
public:
  Physic();
  ~Physic();

  void reset();

  void set_velocity_x(float nvx);
  void set_velocity_y(float nvy);
  void set_velocity(float nvx, float nvy);

  void inverse_velocity_x();
  void inverse_velocity_y();

  float get_velocity_x();
  float get_velocity_y();

  void set_acceleration_x(float nax);
  void set_acceleration_y(float nay);
  void set_acceleration(float nax, float nay);

  float get_acceleration_x();
  float get_acceleration_y();

  void enable_gravity(bool enable_gravity);

  /** Integrate position over frame_ratio frames. */
  void apply(float frame_ratio, float &x, float &y);

private:
  float ax, ay;
  float vx, vy;
  bool gravity_enabled;
};

#endif /* SUPERTUX_PHYSIC_H */

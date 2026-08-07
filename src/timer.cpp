// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform_config.h"
#include "defines.h"
#include "timer.h"

unsigned int st_pause_ticks, st_pause_count;

/* Last raw SDL_GetTicks() seen by st_get_ticks(). Used to detect long stalls
   (hardware suspend, debugger) where the tick counter still advances but the
   game was not simulating — those gaps must not drain level time_left. */
static unsigned int st_last_raw_ticks = 0;

/* Gaps longer than this (ms) while not in an explicit pause are treated as
   suspended time and folded into st_pause_ticks. Normal frames are ~10–50ms. */
static const unsigned int ST_SUSPEND_GAP_MS = 500;

unsigned int st_get_ticks(void)
{
  unsigned int raw = SDL_GetTicks();

  /* Auto-exclude long wall-clock jumps when the pause menu is not already
     holding the clock. Explicit pause (st_pause_count != 0) already freezes
     the return value; updating st_last_raw_ticks there avoids double-counting
     when the menu is closed. */
  if (st_last_raw_ticks != 0 && st_pause_count == 0)
    {
      unsigned int gap = raw - st_last_raw_ticks;
      if (gap > ST_SUSPEND_GAP_MS)
        st_pause_ticks += gap;
    }
  st_last_raw_ticks = raw;

  /* st_pause_ticks accumulates time spent while paused / suspended.
     st_pause_count is the wall-clock moment pause started (0 = not paused).
     While paused, freeze the returned clock at the pause start. */
  if (st_pause_count != 0)
    return st_pause_count - st_pause_ticks;
  else
    return raw - st_pause_ticks;
}

void st_pause_ticks_init(void)
{
  st_pause_ticks = 0;
  st_pause_count = 0;
  st_last_raw_ticks = 0;
}

void st_pause_ticks_start(void)
{
  if(st_pause_count == 0)
    st_pause_count = SDL_GetTicks();
}

void st_pause_ticks_stop(void)
{
  if(st_pause_count == 0)
    return;

  st_pause_ticks += SDL_GetTicks() - st_pause_count;
  st_pause_count = 0;
}

bool st_pause_ticks_started(void)
{
  return st_pause_count != 0;
}

Timer::Timer()
{
  init(true);
}

void
Timer::init(bool st_ticks)
{
  period    = 0;
  time      = 0;
  get_ticks = st_ticks ? st_get_ticks : SDL_GetTicks;
}

void
Timer::start(unsigned int period_)
{
  time   = get_ticks();
  period = period_;
}

void
Timer::stop()
{
  if(get_ticks == st_get_ticks)
    init(true);
  else
    init(false);
}

int
Timer::check()
{
  if((time != 0) && (time + period > get_ticks()))
    return true;
  else
    {
      time = 0;
      return false;
    }
}

int
Timer::started()
{
  if(time != 0)
    return true;
  else
    return false;
}

int
Timer::get_left()
{
  return (period - (get_ticks() - time));
}

int
Timer::get_gone()
{
  return (get_ticks() - time);
}

void
Timer::fwrite(FILE* fi)
{
  unsigned int diff_ticks;
  int tick_mode;
  if(time != 0)
    diff_ticks = get_ticks() - time;
  else
    diff_ticks = 0;

  ::fwrite(&period,sizeof(unsigned int),1,fi);
  ::fwrite(&diff_ticks,sizeof(unsigned int),1,fi);
  if(get_ticks == st_get_ticks)
      tick_mode = true;
  else
      tick_mode = false;
  ::fwrite(&tick_mode,sizeof(unsigned int),1,fi);
}

void
Timer::fread(FILE* fi)
{
  unsigned int diff_ticks;
  int tick_mode;

  if (::fread(&period,sizeof(unsigned int),1,fi) != 1 ||
      ::fread(&diff_ticks,sizeof(unsigned int),1,fi) != 1 ||
      ::fread(&tick_mode,sizeof(unsigned int),1,fi) != 1)
    {
      period = 0;
      diff_ticks = 0;
      tick_mode = 0;
    }

  if (tick_mode)
    get_ticks = st_get_ticks;
  else
    get_ticks = SDL_GetTicks;

  if (diff_ticks != 0)
    time = get_ticks() - diff_ticks;
  else
    time = 0;

}

/* EOF */

// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-FileCopyrightText: 2004 Ingo Ruhnke <grumbel@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "platform_config.h"

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "defines.h"
#include "globals.h"
#include "gameloop.h"
#include "screen.h"
#include "setup.h"
#include "high_scores.h"
#include "menu.h"
#include "touch_controls.h"
#include "badguy.h"
#include "world.h"
#include "special.h"
#include "player.h"
#include "level.h"
#include "scene.h"
#include "collision.h"
#include "tile.h"
#include "particlesystem.h"
#include "resources.h"
#ifndef NOSOUND
#include "music_manager.h"
#endif

GameSession* GameSession::current_ = 0;

GameSession::GameSession(const std::string& subset_, int levelnb_, int mode)
  : world(0), st_gl_mode(mode), levelnb(levelnb_), end_sequence(NO_ENDSEQUENCE),
    overlay(OVERLAY_NONE), overlay_min_ms(0), pending_exit(ES_NONE),
    subset(subset_)
{
  fps_cnt = 0;
  current_ = this;
  
  global_frame_counter = 0;
  game_pause = false;

  fps_timer.init(true);            
  frame_timer.init(true);
  overlay_timer.init(true);

  restart_level();

#ifdef TSCONTROL
  old_mouse_y = screen->h;
#endif
}

void
GameSession::restart_level()
{
  game_pause   = false;
  exit_status  = ES_NONE;
  end_sequence = NO_ENDSEQUENCE;

  fps_timer.init(true);
  frame_timer.init(true);

  float old_x_pos = -1;

  if (world)
    { // Tux has lost a life, so we try to respawn him at the nearest reset point
      old_x_pos = world->get_tux()->base.x;
    }
  
  delete world;

  if (st_gl_mode == ST_GL_LOAD_LEVEL_FILE)
    {
      world = new World(subset);
    }
  else if (st_gl_mode == ST_GL_DEMO_GAME)
    {
      world = new World(subset);
    }
  else
    {
      world = new World(subset, levelnb);
    }

  // Set Tux to the nearest reset point
  if (old_x_pos != -1)
    {
      ResetPoint best_reset_point = { -1, -1 };
      for(std::vector<ResetPoint>::iterator i = get_level()->reset_points.begin();
          i != get_level()->reset_points.end(); ++i)
        {
          if (i->x - screen->w/2 < old_x_pos && best_reset_point.x < i->x)
            best_reset_point = *i;
        }
      
      if (best_reset_point.x != -1)
        {
          world->get_tux()->base.x = best_reset_point.x;
          world->get_tux()->base.y = best_reset_point.y;
          world->get_tux()->old_base = world->get_tux()->base;
          world->get_tux()->previous_base = world->get_tux()->base;

          if(collision_object_map(world->get_tux()->base)) {
              std::cout << "Warning: reset point inside a wall.\n";
          }                                                                  

          scroll_x = best_reset_point.x - screen->w/2;
        }
    }
    
  overlay = OVERLAY_NONE;
  pending_exit = ES_NONE;
  overlay_timer.stop();

  if (st_gl_mode != ST_GL_DEMO_GAME
      && (st_gl_mode == ST_GL_PLAY || st_gl_mode == ST_GL_LOAD_LEVEL_FILE))
    {
      /* Non-blocking intro: process_overlay() finishes it then starts timers. */
      levelintro();
    }
  else
    {
      time_left.init(true);
      start_timers();
#ifndef NOSOUND
      world->play_music(LEVEL_MUSIC);
#endif
    }
}

GameSession::~GameSession()
{
  delete world;
}

void
GameSession::draw_levelintro(void)
{
  char str[60];

  if (get_level()->img_bkgd)
    get_level()->img_bkgd->draw(0, 0);
  else
    drawgradient(get_level()->bkgd_top, get_level()->bkgd_bottom);

  sprintf(str, "%s", world->get_level()->name.c_str());
  gold_text->drawf(str, 0, 200, A_HMIDDLE, A_TOP, 1);

  sprintf(str, "TUX x %d", player_status.lives);
  white_text->drawf(str, 0, 224, A_HMIDDLE, A_TOP, 1);

  sprintf(str, "by %s", world->get_level()->author.c_str());
  white_small_text->drawf(str, 0, 360, A_HMIDDLE, A_TOP, 1);
}

void
GameSession::levelintro(void)
{
#ifndef NOSOUND
  music_manager->halt_music();
#endif
  /* Frame-driven: min 1s, max 3s (same as historic wait_for_event). */
  overlay = OVERLAY_INTRO;
  overlay_min_ms = 1000;
  overlay_timer.start(3000);
  pending_exit = ES_NONE;
}

/* Reset Timers */
void
GameSession::start_timers()
{
  st_pause_ticks_init();
  time_left.start(world->get_level()->time_left*1000);
  update_time = st_get_ticks();
}

void
GameSession::on_escape_press()
{
  if(game_pause)
    return;

  if(st_gl_mode == ST_GL_TEST)
    {
      exit_status = ES_LEVEL_ABORT;
    }
  else if (!Menu::current())
    {
      Menu::set_current(game_menu);
      st_pause_ticks_start();
    }
}

void
GameSession::process_events()
{
  if (end_sequence != NO_ENDSEQUENCE)
    {
      Player& tux = *world->get_tux();
         
      tux.input.fire  = UP;
      tux.input.left  = UP;
      tux.input.right = DOWN;
      tux.input.down  = UP; 

      if (int(last_x_pos) == int(tux.base.x))
        tux.input.up    = DOWN; 
      else
        tux.input.up    = UP; 

      last_x_pos = tux.base.x;

      SDL_Event event;
      while (SDL_PollEvent(&event))
        {
          /* Check for menu-events, if the menu is shown */
          if (Menu::current())
            {
              Menu::current()->event(event);
	      if(!Menu::current())
	      st_pause_ticks_stop();
            }

          switch(event.type)
            {
            case SDL_QUIT:        /* Quit event - quit: */
              st_abort("Received window close", "");
              break;
              
            case SDL_KEYDOWN:     /* A keypress! */
              {
                SDLKey key = event.key.keysym.sym;
                if (st_is_escape_key(key))
                  on_escape_press();
              }
              break;

#ifdef USE_SDL2
            case SDL_CONTROLLERBUTTONDOWN:
              if ((int)event.cbutton.button == gamecontroller_keymap.menu
                  || (int)event.cbutton.button == gamecontroller_keymap.menu_alt)
                on_escape_press();
              break;
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
              st_gamepad_process_device_event(event);
              break;
#else
            case SDL_JOYBUTTONDOWN:
              if (event.jbutton.button == joystick_keymap.start_button)
                on_escape_press();
              break;
#endif
            }
        }
    }
  else // normal mode
    {
      if(!Menu::current() && !game_pause)
        st_pause_ticks_stop();

      SDL_Event event;
      while (SDL_PollEvent(&event))
        {
          bool want_escape = false;
          bool routed = touch_controls_process_event(event, &want_escape);

          if (want_escape)
            {
              on_escape_press();
              continue;
            }

          if (routed)
            {
              if (!Menu::current())
                st_pause_ticks_stop();

              /* Menu or pad owned the event — clear player keys so nothing sticks. */
              Player& tux = *world->get_tux();
              tux.key_event((SDLKey)keymap.jump, UP);
              tux.key_event((SDLKey)keymap.duck, UP);
              tux.key_event((SDLKey)keymap.left, UP);
              tux.key_event((SDLKey)keymap.right, UP);
              tux.key_event((SDLKey)keymap.fire, UP);
              continue;
            }

          {
              Player& tux = *world->get_tux();
  
              switch(event.type)
                {
#ifndef GP2X
                case SDL_QUIT:        /* Quit event - quit: */
                  st_abort("Received window close", "");
                  break;

#ifdef USE_SDL2
                case SDL_WINDOWEVENT:
                  /* Focus loss leaves keys stuck DOWN without matching KEYUP. */
                  if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST
                      || event.window.event == SDL_WINDOWEVENT_MINIMIZED)
                    {
                      tux.key_event((SDLKey)keymap.jump, UP);
                      tux.key_event((SDLKey)keymap.duck, UP);
                      tux.key_event((SDLKey)keymap.left, UP);
                      tux.key_event((SDLKey)keymap.right, UP);
                      tux.key_event((SDLKey)keymap.fire, UP);
                      touch_controls_reset();
                    }
                  break;
#endif

                case SDL_KEYDOWN:     /* A keypress! */
                  {
                    SDLKey key = event.key.keysym.sym;
            
                    if(tux.key_event(key,DOWN))
                      break;
                  }
                  break;
                case SDL_KEYUP:      /* A keyrelease! */
                  {
                    SDLKey key = event.key.keysym.sym;

                    if(tux.key_event(key, UP))
                      break;

                    switch(key)
                      {
                      case SDLK_p:
                        if(!Menu::current())
                          {
                            if(game_pause)
                              {
                                game_pause = false;
                                st_pause_ticks_stop();
                              }
                            else
                              {
                                game_pause = true;
                                st_pause_ticks_start();
                              }
                          }
                        break;
                      case SDLK_TAB:
                        if(debug_mode)
                          {
                            tux.size = !tux.size;
                            if(tux.size == BIG)
                              {
                                tux.base.height = 64;
                              }
                            else
                              tux.base.height = 32;
                          }
                        break;
                      case SDLK_END:
                        if(debug_mode)
                          player_status.distros += 50;
                        break;
                      case SDLK_DELETE:
                        if(debug_mode)
                          tux.got_coffee = 1;
                        break;
                      case SDLK_INSERT:
                        if(debug_mode)
                          tux.invincible_timer.start(TUX_INVINCIBLE_TIME);
                        break;
                      case SDLK_l:
                        if(debug_mode)
                          --player_status.lives;
                        break;
                      case SDLK_s:
                        if(debug_mode)
                          player_status.score += 1000;
                      case SDLK_f:
                        if(debug_fps)
                          debug_fps = false;
                        else
                          debug_fps = true;
                        break;
                      default:
                        break;
                      }
                  }
                  break;
#ifdef TSCONTROL
		case SDL_MOUSEBUTTONDOWN:
		  tux.input.fire = DOWN;
		  break;
		case SDL_MOUSEBUTTONUP:
		  tux.input.fire = UP;
		  break;
		case SDL_MOUSEMOTION:
		  if (event.motion.y < old_mouse_y - 16) {
			tux.input.up = DOWN;
		  }
		  else if (event.motion.y > old_mouse_y + 2) {
	        tux.input.up = UP;
		  }
		  old_mouse_y = event.motion.y;
		  //stand still
		  if ((event.motion.x < (screen->w/2)+(screen->w/10))
		    && (event.motion.x > (screen->w/2)-(screen->w/10))) {
		      tux.input.fire = UP;
		      tux.input.left = UP;
			  tux.input.right = UP;
		  }
		  //run left
		  else if ((event.motion.x > 0) && (event.motion.x < (screen->w/8))) {
		    tux.input.fire = DOWN;
		    tux.input.left = DOWN;
		    tux.input.right = UP;
		  }
		  //walk left
		  else if ((event.motion.x > (screen->w/8)) && (event.motion.x < (screen->w/2))) {
		    tux.input.fire = UP;
		    tux.input.right = UP;
		    tux.input.left = DOWN;
		  }
		  //walk right
		  else if ((event.motion.x > (screen->w/2)) && (event.motion.x < ((7*screen->w)/8))) {
		    tux.input.fire = UP;
		    tux.input.right = DOWN;
		    tux.input.left = UP;
		  }
		  //run right
		  else if ((event.motion.x > ((7*screen->w)/8)) && (event.motion.x < screen->w)) {
		    tux.input.fire = DOWN;
		    tux.input.right = DOWN;
		    tux.input.left = UP;
		  }
		  break;
#endif
#ifdef USE_SDL2
                case SDL_CONTROLLERDEVICEADDED:
                case SDL_CONTROLLERDEVICEREMOVED:
                  st_gamepad_process_device_event(event);
                  break;

                case SDL_CONTROLLERAXISMOTION:
                  if (!use_joystick)
                    break;
                  {
                    const int dz = 16000;
                    if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
                      {
                        if (event.caxis.value < -dz)
                          {
                            tux.input.left  = DOWN;
                            tux.input.right = UP;
                          }
                        else if (event.caxis.value > dz)
                          {
                            tux.input.left  = UP;
                            tux.input.right = DOWN;
                          }
                        else
                          {
                            tux.input.left  = UP;
                            tux.input.right = UP;
                          }
                      }
                    else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
                      {
                        if (event.caxis.value > dz)
                          tux.input.down = DOWN;
                        else
                          tux.input.down = UP;
                      }
                  }
                  break;

                case SDL_CONTROLLERBUTTONDOWN:
                  if (!use_joystick)
                    break;
                  {
                    int b = (int)event.cbutton.button;
                    if (b == gamecontroller_keymap.jump
                        || b == gamecontroller_keymap.up)
                      tux.input.up = DOWN;
                    if (b == gamecontroller_keymap.fire
                        || b == gamecontroller_keymap.fire_alt)
                      tux.input.fire = DOWN;
                    if (b == gamecontroller_keymap.duck)
                      tux.input.down = DOWN;
                    if (b == gamecontroller_keymap.left)
                      {
                        tux.input.left = DOWN;
                        tux.input.right = UP;
                      }
                    if (b == gamecontroller_keymap.right)
                      {
                        tux.input.right = DOWN;
                        tux.input.left = UP;
                      }
                    if (b == gamecontroller_keymap.menu
                        || b == gamecontroller_keymap.menu_alt)
                      on_escape_press();
                  }
                  break;

                case SDL_CONTROLLERBUTTONUP:
                  if (!use_joystick)
                    break;
                  {
                    int b = (int)event.cbutton.button;
                    if (b == gamecontroller_keymap.jump
                        || b == gamecontroller_keymap.up)
                      tux.input.up = UP;
                    if (b == gamecontroller_keymap.fire
                        || b == gamecontroller_keymap.fire_alt)
                      tux.input.fire = UP;
                    if (b == gamecontroller_keymap.duck)
                      tux.input.down = UP;
                    if (b == gamecontroller_keymap.left)
                      tux.input.left = UP;
                    if (b == gamecontroller_keymap.right)
                      tux.input.right = UP;
                  }
                  break;
#else
		case SDL_JOYHATMOTION:
		  if (!use_joystick)
		    break;
		  if ((event.jhat.value == SDL_HAT_RIGHT) || 
		      (event.jhat.value == SDL_HAT_RIGHTUP) ){
			tux.input.left  = UP;
			tux.input.right = DOWN;
		  }
		  if ((event.jhat.value == SDL_HAT_LEFT) ||
		      (event.jhat.value == SDL_HAT_LEFTUP) ){
			tux.input.left  = DOWN;
			tux.input.right = UP;
		  }
		  if (event.jhat.value == SDL_HAT_CENTERED) {
                        tux.input.left  = DOWN;
			tux.input.right = DOWN;
                  }
		 
		  if ( (event.jhat.value ==  ( SDL_HAT_DOWN)) ||
		       (event.jhat.value ==  ( SDL_HAT_LEFTDOWN)) ||
		       (event.jhat.value ==  ( SDL_HAT_RIGHTDOWN)) )
			 tux.input.down = DOWN;

		  if ((event.jhat.value != ( SDL_HAT_DOWN)) && 
                      (event.jhat.value != ( SDL_HAT_LEFTDOWN)) &&
                      (event.jhat.value != ( SDL_HAT_RIGHTDOWN)))
			 tux.input.down = UP;
                           
                  break;


                case SDL_JOYAXISMOTION:
                  if (!use_joystick)
                    break;
                  if (event.jaxis.axis == joystick_keymap.x_axis)
                    {
                      if (event.jaxis.value < -joystick_keymap.dead_zone)
                        {
                          tux.input.left  = DOWN;
                          tux.input.right = UP;
                        }
                      else if (event.jaxis.value > joystick_keymap.dead_zone)
                        {
                          tux.input.left  = UP;
                          tux.input.right = DOWN;
                        }
                      else
                        {
                          tux.input.left  = DOWN;
                          tux.input.right = DOWN;
                        }
                    }
                  else if (event.jaxis.axis == joystick_keymap.y_axis)
                    {
                      if (event.jaxis.value > joystick_keymap.dead_zone)
                        tux.input.down = DOWN;
                      else if (event.jaxis.value < -joystick_keymap.dead_zone)
                        tux.input.down = UP;
                      else
                        tux.input.down = UP;
                    }
                  break;
#endif            
                case SDL_JOYBUTTONDOWN:
#ifndef GP2X
                  if (!use_joystick)
                    break;
                  if (event.jbutton.button == joystick_keymap.a_button)
                    tux.input.up = DOWN;
                  else if (event.jbutton.button == joystick_keymap.b_button)
                    tux.input.fire = DOWN;
                  else if (event.jbutton.button == joystick_keymap.start_button)
                    on_escape_press();
                  break;
#else
                  if (event.jbutton.button == joystick_keymap.a_button)
                    tux.input.up = DOWN;
                  else if (event.jbutton.button == joystick_keymap.b_button)
                    tux.input.fire = DOWN;
                  else if (event.jbutton.button == joystick_keymap.start_button)
                    on_escape_press();
                  else if (event.jbutton.button == joystick_keymap.up_button)
                    tux.input.up = DOWN;
                  else if (event.jbutton.button == joystick_keymap.down_button)
                    tux.input.down = DOWN;
                  else if (event.jbutton.button == joystick_keymap.right_button)
                    tux.input.right = DOWN;
                  else if (event.jbutton.button == joystick_keymap.left_button)
                    tux.input.left = DOWN;
#ifndef NOSOUND
 				  else if (event.jbutton.button == joystick_keymap.voldown_button)
		    decreaseSoundVolume();
                  else if (event.jbutton.button == joystick_keymap.volup_button)
		    increaseSoundVolume();
#endif
                  break;
#endif

                case SDL_JOYBUTTONUP:
#ifndef GP2X
                  if (!use_joystick)
                    break;
                  if (event.jbutton.button == joystick_keymap.a_button)
                    tux.input.up = UP;
                  else if (event.jbutton.button == joystick_keymap.b_button)
                    tux.input.fire = UP;
                  break;

#else
                  if (event.jbutton.button == joystick_keymap.a_button)
                    tux.input.up = UP;
                  else if (event.jbutton.button == joystick_keymap.b_button)
                    tux.input.fire = UP;
                  else if (event.jbutton.button == joystick_keymap.up_button)
                    tux.input.up = UP;
                  else if (event.jbutton.button == joystick_keymap.down_button)
                    tux.input.down = UP;
                  else if (event.jbutton.button == joystick_keymap.right_button)
                    tux.input.right = UP;
                  else if (event.jbutton.button == joystick_keymap.left_button)
                    tux.input.left = UP;
                  break;
#endif
#endif
                default:
                  break;
                }  /* switch */
            }
        } /* while */
    }
}

void
GameSession::check_end_conditions()
{
  Player* tux = world->get_tux();

  /* End of level? */
  int endpos = (World::current()->get_level()->width-5) * (32);
  Tile* endtile = collision_goal(tux->base);

  // fallback in case the other endpositions don't trigger
  if (!end_sequence && tux->base.x >= endpos)
    {
      end_sequence = ENDSEQUENCE_WAITING;
      last_x_pos = -1;
#ifndef NOSOUND
      music_manager->play_music(level_end_song, 0);
#endif
      endsequence_timer.start(7000);
      tux->invincible_timer.start(7000); //FIXME: Implement a winning timer for the end sequence (with special winning animation etc.)
    }
  else if(end_sequence && !endsequence_timer.check())
    {
      exit_status = ES_LEVEL_FINISHED;
      return;
    }
  else if(end_sequence == ENDSEQUENCE_RUNNING && endtile && endtile->data >= 1)
    {
      end_sequence = ENDSEQUENCE_WAITING;
    }
  else if(!end_sequence && endtile && endtile->data == 0)
    {
      end_sequence = ENDSEQUENCE_RUNNING;
      last_x_pos = -1;
#ifndef NOSOUND
      music_manager->play_music(level_end_song, 0);
#endif
	  endsequence_timer.start(7000); // 5 seconds until we finish the map
      tux->invincible_timer.start(7000); //FIXME: Implement a winning timer for the end sequence (with special winning animation etc.)
    }
  else if (!end_sequence && tux->is_dead())
    {
      player_status.bonus = PlayerStatus::NO_BONUS;

      if (player_status.lives < 0)
        { // No more lives!?
          if(st_gl_mode != ST_GL_TEST)
            {
              drawendscreen(); /* sets overlay + pending_exit */
            }
          else
            exit_status = ES_GAME_OVER;
        }
      else
        { // Still has lives, so reset Tux to the levelstart
          restart_level();
        }

      return;
    }
}

void
GameSession::action(double frame_ratio)
{
  if (exit_status == ES_NONE)
    {
      if (!Menu::current() && !game_pause && world)
        touch_controls_apply_player(*world->get_tux());
      // Update Tux and the World
      world->action(frame_ratio);
    }
}

void 
GameSession::draw()
{
  world->draw();
  drawstatus();

  if(game_pause)
    {
      int x = screen->h / 20;
      for(int i = 0; i < x; ++i)
        {
          fillrect(i % 2 ? (pause_menu_frame * i)%screen->w : -((pause_menu_frame * i)%screen->w) ,(i*20+pause_menu_frame)%screen->h,screen->w,10,20,20,20, rand() % 20 + 1);
        }
      fillrect(0,0,screen->w,screen->h,rand() % 50, rand() % 50, rand() % 50, 128);
      blue_text->drawf("PAUSE - Press 'P' To Play", 0, 230, A_HMIDDLE, A_TOP, 1);
    }

  if(Menu::current())
    {
      Menu::current()->draw();
      mouse_cursor->draw();
    }
  /* Overlay: full pad while playing; menu button alone is always useful. */
  if (!game_pause)
    touch_controls_draw();
  
#ifdef TSCONTROL
  if (show_mouse) MouseCursor::current()->draw();
  int y = 5*screen->h/6;
  int h = screen->h/6;
  //run left
  fillrect(
    0,
	y,
	screen->w/8,
	h,
	20,20,20,
	60
  );
  //walk left
  fillrect(
    screen->w/8,
	y,
	screen->w/2 - screen->w/10 - screen->w/8,
	h,
	20,20,20,
	40
  );
  //stand
  fillrect(
    screen->w/2 - (screen->w/10),
	y,
	screen->w/5,
	h,
	20,20,20,
	20
  );
  //walk right
  fillrect(
    screen->w/2 + (screen->w/10),
	y,
	screen->w/2 - screen->w/10 - screen->w/8,
	h,
	20,20,20,
	40
  );
  //run right
  fillrect(
    7*screen->w/8,
	y,
	screen->w/8,
	h,
	20,20,20,
	60
  );
#endif

#ifndef NOSOUND
#ifdef GP2X
  updateSound();
#endif
#endif
  updatescreen();
}

void
GameSession::process_menu()
{
  Menu* menu = Menu::current();
  if(menu)
    {
      menu->action();

      if(menu == game_menu)
        {
          switch (game_menu->check())
            {
            case MNID_CONTINUE:
              st_pause_ticks_stop();
              break;
            case MNID_ABORTLEVEL:
              st_pause_ticks_stop();
              touch_controls_reset();
              exit_status = ES_LEVEL_ABORT;
              break;
            }
        }
      else if(menu == options_menu)
        {
          process_options_menu();
        }
      else if(menu == load_game_menu )
        {
          process_load_game_menu();
        }
    }
}

void
GameSession::begin_run()
{
  Menu::set_current(0);
  current_ = this;

  fps_cnt = 0;
  exit_status = ES_NONE;
  update_time = last_update_time = st_get_ticks();

  // Eat unneeded events
  SDL_Event event;
  while (SDL_PollEvent(&event)) {}

  draw();
}

GameSession::ExitStatus
GameSession::run()
{
  begin_run();

  while (frame())
    { /* busy loop — frame() does one iteration */ }

  return exit_status;
}


bool
GameSession::process_overlay()
{
  if (overlay == OVERLAY_NONE)
    return false;

  /* Draw overlay content. */
  if (overlay == OVERLAY_INTRO)
    draw_levelintro();
  else if (overlay == OVERLAY_ENDSCREEN)
    draw_endscreen_content();
  else if (overlay == OVERLAY_RESULT)
    draw_resultscreen_content();

  flipscreen();

  /* Input: skip after minimum duration (same semantics as wait_for_event). */
  SDL_Event event;
  bool skip = false;
  while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
        {
          exit_status = ES_LEVEL_ABORT;
          overlay = OVERLAY_NONE;
          return false;
        }
      if (event.type == SDL_KEYDOWN
          || event.type == SDL_JOYBUTTONDOWN
          || event.type == SDL_MOUSEBUTTONDOWN
#ifdef USE_SDL2
          || event.type == SDL_FINGERDOWN
          || event.type == SDL_CONTROLLERBUTTONDOWN
#endif
         )
        {
          if (overlay_timer.get_gone() >= (int)overlay_min_ms)
            skip = true;
        }
    }

  if (skip || !overlay_timer.check())
    {
      OverlayKind done = overlay;
      overlay = OVERLAY_NONE;
      overlay_timer.stop();

      if (done == OVERLAY_INTRO)
        {
          time_left.init(true);
          start_timers();
#ifndef NOSOUND
          world->play_music(LEVEL_MUSIC);
#endif
          update_time = last_update_time = st_get_ticks();
        }
      else if (pending_exit != ES_NONE)
        {
          exit_status = static_cast<ExitStatus>(pending_exit);
          pending_exit = ES_NONE;
        }
      return (exit_status == ES_NONE);
    }

  st_frame_delay(20);
  return true; /* still in overlay — keep session running */
}

bool
GameSession::frame()
{
  if (exit_status != ES_NONE)
    return false;

  if (process_overlay())
    return true;
  if (exit_status != ES_NONE)
    return false;

#ifdef GP2X
  st_frame_delay(10);
#endif
  /* Calculate the movement-factor */
  double frame_ratio = ((double)(update_time-last_update_time))/((double)FRAME_RATE);

  if(!frame_timer.check())
    {
      frame_timer.start(25);
      ++global_frame_counter;
    }

  /* Handle events: */
  world->get_tux()->input.old_fire = world->get_tux()->input.fire;

  process_events();
  process_menu();

  // Update the world state and all objects in the world
  // Do that with a constante time-delta so that the game will run
  // determistic and not different on different machines
  if(!game_pause && !Menu::current())
    {
      // Update the world
      check_end_conditions();
      if (end_sequence == ENDSEQUENCE_RUNNING)
         action(frame_ratio/2);
      else if(end_sequence == NO_ENDSEQUENCE)
         action(frame_ratio);
    }
  else
    {
      ++pause_menu_frame;
      st_frame_delay(50);
    }

  draw();

  /* Time stops in pause mode */
  if(game_pause || Menu::current())
    return (exit_status == ES_NONE);

  /* Set the time of the last update and the time of the current update */
  last_update_time = update_time;
  update_time      = st_get_ticks();

  /* Pause till next frame, if the machine running the game is too fast: */
  /* FIXME: Works great for in OpenGl mode, where the CPU doesn't have to do that much. But
     the results in SDL mode aren't perfect (thought the 100 FPS are reached), even on an AMD2500+. */
  if(last_update_time >= update_time - 12)
    {
      st_frame_delay(10);
      update_time = st_get_ticks();
    }

  /* Handle time: */
  if (!time_left.check() && world->get_tux()->dying == DYING_NOT
          && !end_sequence)
    world->get_tux()->kill(Player::KILL);

#ifndef NOSOUND
  /* Handle music: */
  if(world->get_tux()->invincible_timer.check() && !end_sequence)
    {
      world->play_music(HERRING_MUSIC);
    }
  /* are we low on time ? */
  else if (time_left.get_left() < TIME_WARNING && !end_sequence)
    {
      world->play_music(HURRYUP_MUSIC);
    }
  /* or just normal music? */
  else if(world->get_music_type() != LEVEL_MUSIC && !end_sequence)
    {
      world->play_music(LEVEL_MUSIC);
    }

#endif
  /* Calculate frames per second */
  if(show_fps)
    {
      ++fps_cnt;
      fps_fps = (1000.0 / (float)fps_timer.get_gone()) * (float)fps_cnt;

      if(!fps_timer.check())
        {
          fps_timer.start(1000);
          fps_cnt = 0;
        }
    }
#ifndef NOSOUND
#ifdef GP2X
  updateSound();
#endif
#endif
  return (exit_status == ES_NONE);
}

/* Bounce a brick: */
void bumpbrick(float x, float y)
{
  World::current()->add_bouncy_brick(((int)(x + 1) / (32)) * (32),
                         (int)(y / (32)) * (32));

#ifndef NOSOUND
#ifndef GP2X
  play_sound(sounds[SND_BRICK], SOUND_CENTER_SPEAKER);
#else
  play_chunk(SND_BRICK);
#endif
#endif
}

/* (Status): */
void
GameSession::drawstatus()
{
 int xdiv;
#ifdef RES320X240
 xdiv=2;
#else
 xdiv=1;
#endif

  char str[60];

  sprintf(str, "%d", player_status.score);
  white_text->draw("SCORE", 0, 0, 1);
  gold_text->draw(str, 96/xdiv, 0, 1);

  if(st_gl_mode == ST_GL_TEST)
    {
      white_text->draw("Press ESC To Return",0,20,1);
    }

  if(!time_left.check()) {
    white_text->draw("TIME'S UP", 224/xdiv, 0, 1);
  } else if (time_left.get_left() > TIME_WARNING || (global_frame_counter % 10) < 5) {
    sprintf(str, "%d", time_left.get_left() / 1000 );
    white_text->draw("TIME", 224/xdiv, 0, 1);
    gold_text->draw(str, 304/xdiv, 0, 1);
  }

  sprintf(str, "%d", player_status.distros);
  white_text->draw("COINS", screen->h, 0, 1);
  gold_text->draw(str, 608/xdiv, 0, 1);

  white_text->draw("LIVES", 480/xdiv, 20);
  if (player_status.lives >= 5)
    {
      sprintf(str, "%dx", player_status.lives);
#ifdef RES320X240
      gold_text->draw_align(str, 617/xdiv-5, 20, A_RIGHT, A_TOP);
      tux_life->draw(565+(18*3)/xdiv+10, 20);
#else
      gold_text->draw_align(str, 617, 20, A_RIGHT, A_TOP);
      tux_life->draw(565+(18*3), 20);
#endif
    }
  else
    {
      for(int i= 0; i < player_status.lives; ++i)
        tux_life->draw(565+(18*i)/xdiv,20);
    }

  if(show_fps)
    {
      sprintf(str, "%2.1f", fps_fps);
      white_text->draw("FPS", screen->h, 40, 1);
      gold_text->draw(str, screen->h + 60, 40, 1);
    }
//    updateSound();
}

void
GameSession::draw_endscreen_content()
{
  char str[80];

  if (get_level()->img_bkgd)
    get_level()->img_bkgd->draw(0, 0);
  else
    drawgradient(get_level()->bkgd_top, get_level()->bkgd_bottom);

  blue_text->drawf("GAMEOVER", 0, 200, A_HMIDDLE, A_TOP, 1);

  sprintf(str, "SCORE: %d", player_status.score);
  gold_text->drawf(str, 0, 224, A_HMIDDLE, A_TOP, 1);

  sprintf(str, "COINS: %d", player_status.distros);
  gold_text->drawf(str, 0, 256, A_HMIDDLE, A_TOP, 1);
}

void
GameSession::drawendscreen()
{
  /* Frame-driven overlay; process_overlay applies pending_exit when done. */
  overlay = OVERLAY_ENDSCREEN;
  overlay_min_ms = 2000;
  overlay_timer.start(5000);
  pending_exit = ES_GAME_OVER;
}

void
GameSession::draw_resultscreen_content()
{
  char str[80];

  if (get_level()->img_bkgd)
    get_level()->img_bkgd->draw(0, 0);
  else
    drawgradient(get_level()->bkgd_top, get_level()->bkgd_bottom);

  blue_text->drawf("Result:", 0, 200, A_HMIDDLE, A_TOP, 1);

  sprintf(str, "SCORE: %d", player_status.score);
  gold_text->drawf(str, 0, 224, A_HMIDDLE, A_TOP, 1);

  sprintf(str, "COINS: %d", player_status.distros);
  gold_text->drawf(str, 0, 256, A_HMIDDLE, A_TOP, 1);
}

void
GameSession::drawresultscreen(void)
{
  overlay = OVERLAY_RESULT;
  overlay_min_ms = 2000;
  overlay_timer.start(5000);
  pending_exit = ES_LEVEL_FINISHED;
}

std::string slotinfo(int slot)
{
  char tmp[1024];
  char slotfile[1024];
  std::string title;
  sprintf(slotfile,"%s/slot%d.stsg",st_save_dir,slot);

  lisp_object_t* savegame = lisp_read_from_file(slotfile);
  if (savegame)
    {
      LispReader reader(lisp_cdr(savegame));
      reader.read_string("title", &title);
      lisp_free(savegame);
    }

  if (access(slotfile, F_OK) == 0)
    {
      if (!title.empty())
        snprintf(tmp,1024,"Slot %d - %s",slot, title.c_str());
      else
        snprintf(tmp, 1024,"Slot %d - Savegame",slot);
    }
  else
    sprintf(tmp,"Slot %d - Free",slot);

  return tmp;
}



// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "platform_config.h"
#include "globals.h"
#include "defines.h"
#include "screen.h"
#include "text.h"
#include "app_loop.h"
#include "setup.h"
#include "menu.h"
#include "touch_controls.h"
#include "player.h"
#include "game_file.h"
#ifndef NOSOUND
#include "sound.h"
#endif

Text::Text(const std::string& file, int kind_, int w_, int h_)
{
  kind = kind_;
  w = w_;
  h = h_;

  int mx, my;
  SDL_Surface *conv;
  int pixels;
  int i;
  
  if(kind == TEXT_TEXT)
    {
      mx = 26;
      my = 3;
    }
  else if(kind == TEXT_NUM)
    {
      mx = 10;
      my = 1;
    }
  else
    {
      mx = 0;
      my = 0;
    }

  chars = new Surface(file, USE_ALPHA);
  if (!chars || !chars->impl || !chars->impl->get_sdl_surface())
    st_abort("Can't load font surface", file);

  // Load shadow font.
  conv = SDL_DisplayFormatAlpha(chars->impl->get_sdl_surface());
  if (!conv)
    st_abort("Can't convert font surface", file);
  pixels = conv->w * conv->h;
  SDL_LockSurface(conv);
  for(i = 0; i < pixels; ++i)
    {
      Uint32 *p = (Uint32 *)conv->pixels + i;
      *p = *p & conv->format->Amask;
    }
  SDL_UnlockSurface(conv);
  SDL_SetAlpha(conv, SDL_SRCALPHA, 128);
  shadow_chars = new Surface(conv, USE_ALPHA);

  SDL_FreeSurface(conv);
}

Text::~Text()
{
  delete chars;
  delete shadow_chars;
}

void
Text::draw(const  char* text, int x, int y, int shadowsize, int update, int text_lift)
{
  if(text != NULL)
    {
      if(shadowsize != 0)
        draw_chars(shadow_chars, text,x+shadowsize,y+shadowsize, update);

      draw_chars(chars, text,x,y - text_lift, update);
    }
}

void
Text::draw_chars(Surface* pchars,const  char* text, int x, int y, int update)
{
  int i,j,len;

  len = strlen(text);
  int w = this->w;
  int h = this->h;

  if(kind == TEXT_TEXT)
    {
      for( i = 0, j = 0; i < len; ++i,++j)
        {
          if( text[i] >= ' ' && text[i] <= '/')
            pchars->draw_part((int)(text[i] - ' ')*w,  0 , x+(j*w), y, w, h, 255,  update);
          else if( text[i] >= '0' && text[i] <= '?')
            pchars->draw_part((int)(text[i] - '0')*w, h*1, x+(j*w), y, w, h, 255,  update);
          else if ( text[i] >= '@' && text[i] <= 'O')
            pchars->draw_part((int)(text[i] - '@')*w, h*2, x+(j*w), y, w, h, 255,  update);
          else if ( text[i] >= 'P' && text[i] <= '_')
            pchars->draw_part((int)(text[i] - 'P')*w, h*3, x+(j*w), y, w, h, 255,  update);
          else if ( text[i] >= '`' && text[i] <= 'o')
            pchars->draw_part((int)(text[i] - '`')*w, h*4, x+(j*w), y, w, h, 255,  update);
          else if ( text[i] >= 'p' && text[i] <= '~')
            pchars->draw_part((int)(text[i] - 'p')*w, h*5, x+(j*w), y, w, h, 255,  update);
          else if ( text[i] == '\n')
            {
              y += h + 2;
#ifdef RES320X240
	    y+=6;
#endif
              j = 0;
            }
        }
    }
  else if(kind == TEXT_NUM)
    {
      for( i = 0, j = 0; i < len; ++i, ++j)
        {
          if ( text[i] >= '0' && text[i] <= '9')
            pchars->draw_part((int)(text[i] - '0')*w, 0, x+(j*w), y, w, h, 255, update);
          else if ( text[i] == '\n')
            {
              y += h + 2;
#ifdef RES320X240
	    y+=6;
#endif
              j = 0;
            }
        }
    }
}

void
Text::draw_align(const char* text, int x, int y,
                      TextHAlign halign, TextVAlign valign, int shadowsize, int update,
                      int text_lift)
{
  if(text != NULL)
    {
      switch (halign)
        {
        case A_RIGHT:
          x += -(strlen(text)*w);
          break;
        case A_HMIDDLE:
          x += -((strlen(text)*w)/2);
          break;
        case A_LEFT:
          // default
          break;
        }

      switch (valign)
        {
        case A_BOTTOM:
          y -= h;
          break;
          
        case A_VMIDDLE:
          y -= h/2;

        case A_TOP:
          // default
          break;
        }

      draw(text, x, y, shadowsize, update, text_lift);
    }
}

void
Text::drawf(const  char* text, int x, int y,
                 TextHAlign halign, TextVAlign valign, int shadowsize, int update)
{
  if(text != NULL)
    {
      if(halign == A_RIGHT)  /* FIXME: this doesn't work correctly for strings with newlines.*/
        x += screen->w - (strlen(text)*w);
      else if(halign == A_HMIDDLE)
        x += screen->w/2 - ((strlen(text)*w)/2);

      if(valign == A_BOTTOM)
        y += screen->h - h;
      else if(valign == A_VMIDDLE)
        y += screen->h/2 - h/2;

      draw(text,x,y,shadowsize, update);
    }
}

/* --- ERASE TEXT: --- */

void
Text::erasetext(const  char * text, int x, int y, Surface * ptexture, int update, int shadowsize)
{
  SDL_Rect dest;

  dest.x = x;
  dest.y = y;
  dest.w = strlen(text) * w + shadowsize;
  dest.h = h;

  if (dest.w > screen->w)
    dest.w = screen->w;

  ptexture->draw_part(dest.x,dest.y,dest.x,dest.y,dest.w,dest.h, 255, update);

  if (update == UPDATE)
    update_rect(screen, dest.x, dest.y, dest.w, dest.h);
}


/* --- ERASE CENTERED TEXT: --- */

void
Text::erasecenteredtext(const  char * text, int y, Surface * ptexture, int update, int shadowsize)
{
  erasetext(text, screen->w / 2 - (strlen(text) * 8), y, ptexture, update, shadowsize);
}


/* --- SCROLL TEXT FUNCTION --- */

#define MAX_VEL     10
#define SPEED_INC   0.01
#define SCROLL      60
#define ITEMS_SPACE 4

/* Scrollable text (credits / intro) — frame helpers + blocking wrapper. */

struct TextFileScroll {
  Surface* surface;
  bool own_surface;
  string_list_type names;
  float scroll;
  float speed;
  int done;
  int length;
  Uint32 lastticks;
  bool active;
};

static TextFileScroll g_textscroll = {};


void display_text_file_begin(const std::string& file, Surface* surface, float scroll_speed, bool own_surface)
{
  if (g_textscroll.active)
    display_text_file_end();

  g_textscroll.surface = surface;
  g_textscroll.own_surface = own_surface;
  g_textscroll.scroll = 0;
  g_textscroll.speed = scroll_speed / 50;
  g_textscroll.done = 0;
  g_textscroll.active = true;
  string_list_init(&g_textscroll.names);

  /* Resolve via open_game_file (APK assets, MEMFS /data preload, install).
     Plain fopen("intro.txt") fails on WASM even when /data/intro.txt exists
     because faccessible() succeeds through open_game_file while fopen does not
     search the preload mount. */
  std::string path = file;
  if (!game_file_exists(path) && !datadir.empty())
    path = datadir + "/" + file;

  std::vector<char> buf;
  char shown[1024];
  snprintf(shown, sizeof(shown), "%s", path.c_str());

  if (game_file_read(path, buf))
    {
      buf.push_back('\0');
      const char* p = &buf[0];
      while (*p)
        {
          const char* line = p;
          while (*p && *p != '\n' && *p != '\r')
            ++p;
          std::string row(line, p - line);
          if (*p == '\r')
            ++p;
          if (*p == '\n')
            ++p;
          string_list_add_item(&g_textscroll.names, row.c_str());
        }
    }
  else
    {
      string_list_add_item(&g_textscroll.names,"File was not found!");
      string_list_add_item(&g_textscroll.names,shown);
      string_list_add_item(&g_textscroll.names,"Shame on the guy, who");
      string_list_add_item(&g_textscroll.names,"forgot to include it");
      string_list_add_item(&g_textscroll.names,"in your SuperTux distribution.");
    }

  g_textscroll.length = g_textscroll.names.num_items;
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  g_textscroll.lastticks = SDL_GetTicks();
}

void display_text_file_end(void)
{
  if (!g_textscroll.active)
    return;
  string_list_free(&g_textscroll.names);
  if (g_textscroll.own_surface)
    {
      delete g_textscroll.surface;
      g_textscroll.surface = 0;
    }
  g_textscroll.active = false;
  SDL_EnableKeyRepeat(0, 0);
  Menu::set_current(main_menu);
}

bool display_text_file_frame(void)
{
  if (!g_textscroll.active || g_textscroll.done)
    {
      if (g_textscroll.active)
        display_text_file_end();
      return false;
    }

  Surface* surface = g_textscroll.surface;
  float& scroll = g_textscroll.scroll;
  float& speed = g_textscroll.speed;
  int& done = g_textscroll.done;
  int length = g_textscroll.length;
  string_list_type& names = g_textscroll.names;
  int y;

  SDL_Event event;
  while(SDL_PollEvent(&event))
    {
      if (touch_controls_event(event))
        {
          if (touch_controls_escape_pressed()
              || touch_controls_held(4)
              || touch_controls_held(5)
              || touch_controls_held(6))
            done = 1;
          continue;
        }
      if (st_is_escape_event(event))
        {
          done = 1;
          continue;
        }
      switch(event.type)
        {
        case SDL_KEYDOWN:
          {
            SDLKey key = event.key.keysym.sym;
            if (key == (SDLKey)keymap.up)
              speed -= SPEED_INC;
            else if (key == (SDLKey)keymap.duck)
              speed += SPEED_INC;
            else if (key == (SDLKey)keymap.jump || key == (SDLKey)keymap.fire)
              {
                if (speed >= 0)
                  scroll += SCROLL;
              }
          }
          break;
#ifdef GP2X
        case SDL_JOYBUTTONDOWN:
          if ( event.jbutton.button == joystick_keymap.down_button ) {
            speed += SPEED_INC;
          }
          if ( event.jbutton.button == joystick_keymap.up_button ) {
            speed -= SPEED_INC;
          }
          if ( event.jbutton.button == joystick_keymap.b_button ) {
            done = 1;
          }
          if ( event.jbutton.button == joystick_keymap.a_button ) {
            scroll += SCROLL;
          }
          break;
#endif
#ifdef USE_SDL2
        case SDL_CONTROLLERBUTTONDOWN:
          if (use_joystick)
            {
              int b = (int)event.cbutton.button;
              if (b == gamecontroller_keymap.up)
                speed -= SPEED_INC;
              else if (b == gamecontroller_keymap.duck)
                speed += SPEED_INC;
              else if (b == gamecontroller_keymap.jump
                       || b == gamecontroller_keymap.fire)
                {
                  if (speed >= 0)
                    scroll += SCROLL;
                }
              else if (b == gamecontroller_keymap.menu)
                done = 1;
            }
          break;
        case SDL_FINGERDOWN:
          done = 1;
          break;
#endif
        case SDL_MOUSEBUTTONDOWN:
          done = 1;
          break;
        case SDL_QUIT:
          done = 1;
          break;
        default:
          break;
        }
    }

  if(speed > MAX_VEL)
    speed = MAX_VEL;
  else if(speed < -MAX_VEL)
    speed = -MAX_VEL;

  surface->draw_bg();

  y = 0;
  for(int i = 0; i < length; i++)
    {
    switch(names.item[i][0])
      {
      case ' ':
        white_small_text->drawf(names.item[i]+1, 0, screen->h+y-int(scroll),
            A_HMIDDLE, A_TOP, 1);
        y += white_small_text->h+ITEMS_SPACE;
#ifdef RES320X240
        y += 6;
#endif
        break;
      case '\t':
        white_text->drawf(names.item[i]+1, 0, screen->h+y-int(scroll),
            A_HMIDDLE, A_TOP, 1);
        y += white_text->h+ITEMS_SPACE;
#ifdef RES320X240
        y += 6;
#endif
        break;
      case '-':
#ifdef RES320X240
        white_text->drawf(names.item[i]+1, 0, screen->h+y-int(scroll), A_HMIDDLE, A_TOP, 3);
#else
        white_big_text->drawf(names.item[i]+1, 0, screen->h+y-int(scroll), A_HMIDDLE, A_TOP, 3);
#endif
        y += white_big_text->h+ITEMS_SPACE;
#ifdef RES320X240
        y += 6;
#endif
        break;
      default:
        blue_text->drawf(names.item[i], 0, screen->h+y-int(scroll),
            A_HMIDDLE, A_TOP, 1);
        y += blue_text->h+ITEMS_SPACE;
#ifdef RES320X240
        y += 6;
#endif
        break;
      }
    }

  touch_controls_draw();
  flipscreen();

  if(screen->h+y-scroll < 0 && 20+screen->h+y-scroll < 0)
    done = 1;

  Uint32 ticks = SDL_GetTicks();
  scroll += speed * (ticks - g_textscroll.lastticks);
  g_textscroll.lastticks = ticks;
  if(scroll < 0)
    scroll = 0;

#ifndef GP2X
  st_frame_delay(10);
#else
  st_frame_delay(2);
#ifndef NOSOUND
  updateSound();
#endif
#endif

  if (done)
    {
      display_text_file_end();
      return false;
    }
  return true;
}

void display_text_file(const std::string& file, Surface* surface, float scroll_speed)
{
  if (app_loop_active())
    {
      fprintf(stderr, "display_text_file: blocked under app_loop (use begin/frame)\n");
      return;
    }
  display_text_file_begin(file, surface, scroll_speed, false);
  while (display_text_file_frame())
    { /* busy loop */ }
}

void display_text_file(const std::string& file, const std::string& surface, float scroll_speed)
{
  if (app_loop_active())
    {
      fprintf(stderr, "display_text_file: blocked under app_loop (use begin/frame)\n");
      return;
    }
  Surface* sur = new Surface(datadir + surface, IGNORE_ALPHA);
  display_text_file_begin(file, sur, scroll_speed, true);
  while (display_text_file_frame())
    { /* busy loop */ }
}




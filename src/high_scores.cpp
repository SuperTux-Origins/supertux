// SPDX-FileCopyrightText: 2004 Adam Czachorowski <gislan@o2.pl>
// SPDX-License-Identifier: GPL-3.0-or-later

/* Open the highscore file: */

#include <string.h>
#include <stdlib.h>

#include "globals.h"
#include "high_scores.h"
#include "menu.h"
#include "screen.h"
#include "texture.h"
#include "setup.h"
#include "lispreader.h"

#ifdef WIN32
const char * highscore_filename = "/st_highscore.dat";
#else
const char * highscore_filename = "/highscore";
#endif

int hs_score;
std::string hs_name; /* highscores global variables*/

/* Load data from high score file: */

void load_hs(void)
{
  hs_score = 100;
  hs_name  = "Grandma";

  FILE * fi;
  lisp_object_t* root_obj = 0;
  fi = fopen(highscore_filename, "r");
  if (fi == NULL)
    {
      perror(highscore_filename);
      return;
    }

  lisp_stream_t stream;
  lisp_stream_init_file (&stream, fi);
  root_obj = lisp_read (&stream);

  if (root_obj->type == LISP_TYPE_EOF || root_obj->type == LISP_TYPE_PARSE_ERROR)
    {
      printf("HighScore: Parse Error in file %s", highscore_filename);
    }


  if (strcmp(lisp_symbol(lisp_car(root_obj)), "supertux-highscore") == 0)
    {
      LispReader reader(lisp_cdr(root_obj));
      reader.read_int("score",  &hs_score);
      reader.read_string("name", &hs_name);
    }
 
  fclose(fi);
  lisp_free(root_obj);
}

static Surface* hs_bkgd = 0;
static bool hs_active = false;

void save_hs_begin(int score)
{
  if (hs_active)
    {
      delete hs_bkgd;
      hs_bkgd = 0;
      hs_active = false;
    }

  hs_bkgd = new Surface(datadir + "/images/highscore/highscore.png", IGNORE_ALPHA);
  hs_score = score;
  Menu::set_current(highscore_menu);

  if(!highscore_menu->item[0].input)
    highscore_menu->item[0].input = (char*) malloc(strlen(hs_name.c_str()) + 1);

  strcpy(highscore_menu->item[0].input,hs_name.c_str());
  hs_active = true;
}

static void
save_hs_write_file(void)
{
  FILE* fi;
  std::string filename = highscore_filename;

  fcreatedir(filename.c_str());
  if(fwriteable(filename.c_str()))
    {
      fi = fopen(filename.c_str(), "w");
      if (fi == NULL)
        {
          perror(filename.c_str());
          return;
        }

      fprintf(fi,";SuperTux HighScores\n");
      fprintf(fi,"(supertux-highscore\n");
      fprintf(fi,"  (name \"%s\")\n", hs_name.c_str());
      fprintf(fi,"  (score \"%i\")\n", hs_score);
      fprintf( fi,")");
      fclose(fi);
    }
}

bool save_hs_frame(void)
{
  if (!hs_active)
    return false;

  char str[80];
  SDL_Event event;

  hs_bkgd->draw_bg();

  blue_text->drawf("Congratulations", 0, 130, A_HMIDDLE, A_TOP, 2, NO_UPDATE);
  blue_text->draw("Your score:", 150, 180, 1, NO_UPDATE);
  sprintf(str, "%d", hs_score);
  yellow_nums->draw(str, 350, 170, 1, NO_UPDATE);

  Menu::current()->draw();
  Menu::current()->action();

  flipscreen();

  while(SDL_PollEvent(&event))
    if(event.type == SDL_KEYDOWN)
      Menu::current()->event(event);

  switch (highscore_menu->check())
    {
    case 0:
      if(highscore_menu->item[0].input != NULL)
        hs_name = highscore_menu->item[0].input;
      break;
    }

  if (!Menu::current())
    {
      save_hs_write_file();
      delete hs_bkgd;
      hs_bkgd = 0;
      hs_active = false;
#ifdef __EMSCRIPTEN__
      st_emscripten_fs_sync(0);
#endif
      return false;
    }

  st_frame_delay(25);
  return true;
}

void save_hs(int score)
{
  save_hs_begin(score);
  while (save_hs_frame())
    { /* busy loop */ }
}

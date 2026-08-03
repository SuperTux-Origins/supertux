// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_TEXT_H
#define SUPERTUX_TEXT_H

#include <string>
#include "texture.h"

void display_text_file(const std::string& file, const std::string& surface, float scroll_speed);
void display_text_file(const std::string& file, Surface* surface, float scroll_speed);
/** Split form for a future non-blocking pump (display_text_file still blocks). */
void display_text_file_begin(const std::string& file, Surface* surface, float scroll_speed);
bool display_text_file_frame(void); /* true while scrolling */
void display_text_file_end(void);

/* Kinds of texts. */
enum {
   TEXT_TEXT,
   TEXT_NUM
};

enum TextHAlign {
   A_LEFT,
   A_HMIDDLE,
   A_RIGHT,
};

enum TextVAlign {
   A_TOP,
   A_VMIDDLE,
   A_BOTTOM,
};

/* Text type */
class Text
{
 public:
  Surface* chars;
  Surface* shadow_chars;
  int kind;
  int w;
  int h;
 public:
  Text(const std::string& file, int kind, int w, int h);
  ~Text();

  void draw(const char* text, int x, int y, int shadowsize = 1, int update = NO_UPDATE);
  void draw_chars(Surface* pchars, const char* text, int x, int y, int update = NO_UPDATE);
  void drawf(const char* text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize, int update = NO_UPDATE);
  void draw_align(const char* text, int x, int y, TextHAlign halign, TextVAlign valign, int shadowsize = 1, int update = NO_UPDATE);
  void erasetext(const char * text, int x, int y, Surface* surf, int update, int shadowsize);
  void erasecenteredtext(const char * text, int y, Surface* surf, int update, int shadowsize);
};

#endif /*SUPERTUX_TEXT_H*/


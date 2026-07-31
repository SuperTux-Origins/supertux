// SPDX-FileCopyrightText: 2003 Ricardo Cruz <rick2@aeiou.pt>
// SPDX-FileCopyrightText: 2003 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

/* leveleditor.h - A built-in level editor for SuperTux */

#ifndef SUPERTUX_LEVELEDITOR_H
#define SUPERTUX_LEVELEDITOR_H

struct square
{
  int x1, y1, x2, y2;
};

/* selection modes */
enum {
  SM_CURSOR,
  SM_SQUARE,
  SM_NONE
};

int leveleditor(char* filename = NULL);
void newlevel(void);
void selectlevel(void);
void le_savelevel();
void editlevel(void);
void testlevel(void);
int le_init(void);
void le_checkevents(void);

#endif /*SUPERTUX_LEVELEDITOR_H*/

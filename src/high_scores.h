// SPDX-FileCopyrightText: 2004 Adam Czachorowski <gislan@o2.pl>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_HIGH_SCORES_H
#define SUPERTUX_HIGH_SCORES_H

#include <stdio.h>

extern int hs_score;
extern std::string hs_name; /* highscores global variables*/

void save_hs(int score);
void save_hs_begin(int score);
bool save_hs_frame(void);
void load_hs();

#endif

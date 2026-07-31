// SPDX-FileCopyrightText: 2003 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_RESOURCES_H
#define SUPERTUX_RESOURCES_H

#ifndef NOSOUND
#include "musicref.h"
#endif

class SpriteManager;
#ifndef NOSOUND
class MusicManager;
#endif

extern Surface* img_waves[3]; 
extern Surface* img_water;
extern Surface* img_pole;
extern Surface* img_poletop;
extern Surface* img_flag[2];
extern Surface* img_cloud[2][4];

extern Surface* img_super_bkgd;

#ifndef NOSOUND
extern MusicRef herring_song;
extern MusicRef level_end_song;
extern MusicManager* music_manager;
#endif

extern SpriteManager* sprite_manager;

void loadshared();
void unloadshared();

#endif

/* EOF */


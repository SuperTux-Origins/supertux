// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-FileCopyrightText: 2004 Duong-Khang NGUYEN <neoneurone@users.sf.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_SOUND_H
#define SUPERTUX_SOUND_H

#include "defines.h"     /* get YES/NO defines */

/*global variable*/
extern bool use_sound;           /* handle sound on/off menu and command-line option */
extern bool use_music;           /* handle music on/off menu and command-line */
extern bool audio_device;        /* != 0: available and initialized */

/* enum of different internal music types */
enum Music_Type {
  NO_MUSIC,
  LEVEL_MUSIC,
  HURRYUP_MUSIC,
  HERRING_MUSIC
};

/* panning effects: terrible :-) ! */
enum Sound_Speaker {
  SOUND_LEFT_SPEAKER = 0,
  SOUND_RIGHT_SPEAKER = 1,
  SOUND_RESERVED_CHANNELS = 2, // 2 channels reserved for left/right speaker
  SOUND_CENTER_SPEAKER = -1
};

/* Sound files: */
enum {
  SND_JUMP,
  SND_BIGJUMP,
  SND_SKID,
  SND_DISTRO,
  SND_HERRING,
  SND_BRICK,
  SND_HURT,
  SND_SQUISH,
  SND_FALL,
  SND_RICOCHET,
  SND_BUMP_UPGRADE,
  SND_UPGRADE,
  SND_EXCELLENT,
  SND_COFFEE,
  SND_SHOOT,
  SND_LIFEUP,
  SND_STOMP,
  SND_KICK,
  SND_EXPLODE,
  SND_TELEPORT,
  NUM_SOUNDS
};

extern const char* soundfilenames[NUM_SOUNDS];

#include <string>
#include <SDL_mixer.h>

/* variables for stocking the sound and music */
#ifndef GP2X
extern Mix_Chunk* sounds[NUM_SOUNDS];
#endif

/* functions handling the sound and music */
int open_audio(int frequency, Uint16 format, int channels, int chunksize);
void close_audio( void );

#ifndef GP2X
Mix_Chunk * load_sound(const std::string& file);
#endif
void free_chunk(Mix_Chunk*chunk);
void play_sound(Mix_Chunk * snd, enum Sound_Speaker whichSpeaker);
void sound_volume ( int vol );

#endif /*SUPERTUX_SOUND_H*/

#ifdef GP2X
void play_chunk(int idx);
static void load_sounds();
void updateSound ( void );
void increaseSoundVolume(void);
void decreaseSoundVolume(void);
#endif

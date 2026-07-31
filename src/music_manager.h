// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-FileCopyrightText: 2004 Duong-Khang NGUYEN <neoneurone@users.sf.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_MUSIC_MANAGER_H
#define HEADER_MUSIC_MANAGER_H
#ifdef GP2X
#include "mikmod.h"
#endif

#include <SDL_mixer.h>
#include <string>
#include <map>

class MusicRef;

/** This class manages a list of music resources and is responsible for playing
 * the music.
 */
class MusicManager
{
public:
  MusicManager();
  ~MusicManager();
    
  MusicRef load_music(const std::string& file);
  bool exists_music(const std::string& filename);
  
  void play_music(const MusicRef& music, int loops = -1);
  void halt_music();

  void enable_music(bool enable);

private:
  friend class MusicRef;
  class MusicResource
  {
  public:
    ~MusicResource();

    MusicManager* manager;
#ifndef GP2X
    Mix_Music* music;
#else
    MODULE *music;
#endif
    
    int refcount;
  };

  void free_music(MusicResource* music);

  std::map<std::string, MusicResource> musics;
  MusicResource* current_music;
  bool music_enabled;
};

#endif


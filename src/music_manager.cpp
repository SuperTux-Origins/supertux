// SPDX-FileCopyrightText: 2004 Ingo Ruhnke <grumbel@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <assert.h>
#include "music_manager.h"
#include "musicref.h"
#include "sound.h"
#include "setup.h"
#include "globals.h"
#include "game_file.h"

MusicManager::MusicManager()
  : current_music(0), music_enabled(true)
{ }

MusicManager::~MusicManager()
{
  if(audio_device)
    Mix_HaltMusic();
}

MusicRef
MusicManager::load_music(const std::string& file)
{
  if(!audio_device)
    return MusicRef(0);

  if(!exists_music(file))
    {
      /* Missing/unsupported codec (e.g. MOD without libxmp) must not kill
         the process — run silent and keep going. */
      fprintf(stderr, "Warning: couldn't load music '%s'%s%s\n",
              file.c_str(),
#ifndef GP2X
              Mix_GetError() ? ": " : "",
              Mix_GetError() ? Mix_GetError() : ""
#else
              "", ""
#endif
              );
      return MusicRef(0);
    }

  std::map<std::string, MusicResource>::iterator i = musics.find(file);
  assert(i != musics.end());
  return MusicRef(& (i->second));
}

bool
MusicManager::exists_music(const std::string& file)
{
  if(!audio_device)
    return true;
  
  // song already loaded?
  std::map<std::string, MusicResource>::iterator i = musics.find(file);
  if(i != musics.end()) {
    return true;                                      
  }
  
#ifndef GP2X
  Mix_Music* song = 0;
  {
    SDL_RWops* rw = open_game_file(file);
    if (!rw)
      {
        st_log("[music] open failed: %s", file.c_str());
      }
    else
      {
        st_vlog("[music] loading: %s\n", file.c_str());
#ifdef USE_SDL2
        song = Mix_LoadMUS_RW(rw, 1);
#else
        song = Mix_LoadMUS_RW(rw);
        if (!song)
          SDL_RWclose(rw);
#endif
        if (!song)
          st_log("[music] Mix_LoadMUS failed: %s",
                  Mix_GetError() ? Mix_GetError() : "(no Mix_GetError)");
      }
  }
#else
  char mfile[100];
  snprintf(mfile,sizeof(mfile),"%s",file.c_str());
  MODULE *song=Player_Load(mfile, 64, 0);
#endif
        
  if(song == 0)
    {
      st_log("[music] not available: %s", file.c_str());
      return false;
    }
  st_vlog("[music] loaded ok: %s\n", file.c_str());

  // insert into music list
  std::pair<std::map<std::string, MusicResource>::iterator, bool> result = 
    musics.insert(
      std::pair<std::string, MusicResource>(file, MusicResource()));
  MusicResource& resource = result.first->second;
  resource.manager = this;
  resource.music = song;

  return true;
}

void
MusicManager::free_music(MusicResource* )
{
  // TODO free music, currently we can't do this since SDL_mixer seems to have
  // some bugs if you load/free alot of mod files.  
}

void
MusicManager::play_music(const MusicRef& musicref, int loops)
{
// printf("loop: %d, musicref: %d\n",loops,musicref.music);
  
  if(!audio_device)
    return;

  if(musicref.music == 0)
    {
      st_log("[music] play skipped (null MusicRef - load failed earlier)");
      return;
    }
  if(current_music == musicref.music)
    return;

  if(current_music)
    current_music->refcount--;
  
  current_music = musicref.music;
  current_music->refcount++;
  
  if(music_enabled)
#ifndef GP2X
    Mix_PlayMusic(current_music->music, loops);
#else
  {
    if ( loops == -1 ) current_music->music->wrap=1;
    Player_Stop();
    Player_Start(current_music->music);
    Player_SetPosition(0);
  }
#endif
}

void
MusicManager::halt_music()
{
  if(!audio_device)
    return;
  
#ifndef GP2X
  Mix_HaltMusic();
#else
  Player_Stop();
#endif
  
  if(current_music) {
    current_music->refcount--;
    if(current_music->refcount == 0)
      free_music(current_music);
      current_music = 0;
  }
}

void
MusicManager::enable_music(bool enable)
{
  if(!audio_device)
    return;

  if(enable == music_enabled)
    return;
  
  music_enabled = enable;
  if(music_enabled == false) {
#ifndef GP2X
    Mix_HaltMusic();
#else
    Player_Stop();
#endif
  } else {
#ifndef GP2X
    Mix_PlayMusic(current_music->music, -1);
#else
    Player_Start(current_music->music);
#endif
  }
}

MusicManager::MusicResource::~MusicResource()
{
  // buggy SDL_mixer :-/
  // Mix_FreeMusic(music);
}


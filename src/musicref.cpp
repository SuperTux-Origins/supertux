// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-FileCopyrightText: 2004 Matthias Braun <matze@braunis.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "musicref.h"

MusicRef::MusicRef()
  : music(0)
{
}

MusicRef::MusicRef(MusicManager::MusicResource* newmusic)
  : music(newmusic)
{
  if(music)
    music->refcount++;
}

MusicRef::~MusicRef()
{
  if(music) {
    music->refcount--;
    if(music->refcount == 0)
      music->manager->free_music(music);
  }
}

MusicRef::MusicRef(const MusicRef& other)
  : music(other.music)
{
  if(music)
    music->refcount++;
}

MusicRef&
MusicRef::operator =(const MusicRef& other)
{
  MusicManager::MusicResource* oldres = music;
  music = other.music;
  if(music)
    music->refcount++;
  if(oldres) {
    oldres->refcount--;
    if(oldres->refcount == 0)
      music->manager->free_music(music);
  }

  return *this;
}


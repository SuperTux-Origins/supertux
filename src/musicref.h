// SPDX-FileCopyrightText: 2004 Matthias Braun <matze@braunis.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_MUSIC_RESOURCE_H
#define HEADER_MUSIC_RESOURCE_H

#include "music_manager.h"

/** This class holds a reference to a music file and maintains a correct
 * refcount for that file.
 */
class MusicRef
{
public:
  MusicRef();
  MusicRef(const MusicRef& other);
  ~MusicRef();

  MusicRef& operator= (const MusicRef& other);

private:
  friend class MusicManager;
  MusicRef(MusicManager::MusicResource* music);
  
  MusicManager::MusicResource* music;
};

#endif


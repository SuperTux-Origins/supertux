//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SUPERTUX_SUPERTUX_GAME_OBJECT_HPP
#define HEADER_SUPERTUX_SUPERTUX_GAME_OBJECT_HPP

#include <memory>
#include <string>
#include <vector>

#include "util/fade_helper.hpp"
#include "util/gettext.hpp"
#include "util/reader_fwd.hpp"
#include "util/uid.hpp"

class DrawingContext;
class ObjectRemoveListener;
class Writer;

/**
    Base class for all the things that make up Levels' Sectors.

    Each sector of a level will hold a list of active GameObject while the
    game is played.

    This class is responsible for:
    - Updating and Drawing the object. This should happen in the update() and
      draw() functions. Both are called once per frame.
    - Providing a safe way to remove the object by calling the remove_me
      functions.
*/
class GameObject
{
  friend class GameObjectManager;

public:
  GameObject();
  GameObject(std::string const& name);
  GameObject(ReaderMapping const& reader);
  virtual ~GameObject();

  /** Called after all objects have been added to the Sector and the
      Sector is fully constructed. If objects refer to other objects
      by name, those connection can be resolved here. */
  virtual void finish_construction() {}

  UID get_uid() const { return m_uid; }

  /** This function is called once per frame and allows the object to
      update it's state. The dt_sec is the time that has passed since
      the last frame in seconds and should be the base for all timed
      calculations (don't use SDL_GetTicks directly as this will fail
      in pause mode). This function is not called in the Editor. */
  virtual void update(float dt_sec) = 0;

  /** The GameObject should draw itself onto the provided
      DrawingContext if this function is called. */
  virtual void draw(DrawingContext& context) = 0;

  /** If true only a single object of this type is allowed in a
      given GameObjectManager */
  virtual bool is_singleton() const { return false; }

  /** schedules this object to be removed at the end of the frame */
  virtual void remove_me() { m_scheduled_for_removal = true; }

  /** returns true if the object is not scheduled to be removed yet */
  bool is_valid() const { return !m_scheduled_for_removal; }

  /** registers a remove listener which will be called if the object
      gets removed/destroyed */
  void add_remove_listener(ObjectRemoveListener* listener);

  /** unregisters a remove listener, so it will no longer be called if
      the object gets removed/destroyed */
  void del_remove_listener(ObjectRemoveListener* listener);

  void set_name(std::string const& name) { m_name = name; }
  std::string const& get_name() const { return m_name; }

  /** stops all looping sounds */
  virtual void stop_looping_sounds() {}

  /** continues all looping sounds */
  virtual void play_looping_sounds() {}

private:
  void set_uid(UID const& uid) { m_uid = uid; }

protected:
  /** a name for the gameobject, this is mostly a hint for scripts and
      for debugging, don't rely on names being set or being unique */
  std::string m_name;

  /** Fade Helpers are for easing/fading script functions */
  std::vector<std::unique_ptr<FadeHelper>> m_fade_helpers;

private:
  /** A unique id for the object to safely refer to it. This will be
      set by the GameObjectManager. */
  UID m_uid;

  /** this flag indicates if the object should be removed at the end of the frame */
  bool m_scheduled_for_removal;

  std::vector<ObjectRemoveListener*> m_remove_listeners;

private:
  GameObject(GameObject const&) = delete;
  GameObject& operator=(GameObject const&) = delete;
};

#endif

/* EOF */

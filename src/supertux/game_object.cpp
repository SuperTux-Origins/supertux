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

#include "supertux/game_object.hpp"

#include <algorithm>

#include "supertux/object_remove_listener.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"
#include "video/color.hpp"

GameObject::GameObject() :
  m_name(),
  m_fade_helpers(),
  m_uid(),
  m_scheduled_for_removal(false),
  m_components(),
  m_remove_listeners()
{
}

GameObject::GameObject(std::string const& name) :
  m_name(name),
  m_fade_helpers(),
  m_uid(),
  m_scheduled_for_removal(false),
  m_components(),
  m_remove_listeners()
{
}

GameObject::GameObject(ReaderMapping const& reader) :
  GameObject()
{
  reader.read("name", m_name);
}

GameObject::~GameObject()
{
  for (auto const& entry : m_remove_listeners) {
    entry->object_removed(this);
  }
  m_remove_listeners.clear();
}

void
GameObject::add_remove_listener(ObjectRemoveListener* listener)
{
  m_remove_listeners.push_back(listener);
}

void
GameObject::del_remove_listener(ObjectRemoveListener* listener)
{
  m_remove_listeners.erase(std::remove(m_remove_listeners.begin(),
                                       m_remove_listeners.end(),
                                       listener),
                           m_remove_listeners.end());
}

void
GameObject::update(float dt_sec)
{
  for (auto& h : m_fade_helpers)
  {
    h->update(dt_sec);
  }

  auto new_end = std::remove_if(m_fade_helpers.begin(), m_fade_helpers.end(), [](std::unique_ptr<FadeHelper> const& h) {
    return h->completed();
  });

  m_fade_helpers.erase(new_end, m_fade_helpers.end());
}

/* EOF */

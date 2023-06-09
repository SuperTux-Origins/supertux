//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_GAME_OBJECT_ITERATOR_HPP
#define HEADER_SUPERTUX_SUPERTUX_GAME_OBJECT_ITERATOR_HPP

#include <vector>

#include "game_object_manager.hpp"

template<typename T>
class GameObjectIterator
{
public:
  typedef std::vector<std::unique_ptr<GameObject> >::const_iterator Iterator;

public:
  GameObjectIterator(Iterator it, Iterator end) :
    m_it(it),
    m_end(end),
    m_object()
  {
    if (m_it != m_end)
    {
      m_object = dynamic_cast<T*>(m_it->get());
      if (!m_object)
      {
        skip_to_next();
      }
    }
  }

  GameObjectIterator& operator++()
  {
    skip_to_next();
    return *this;
  }

  GameObjectIterator operator++(int)
  {
    GameObjectIterator tmp(*this);
    skip_to_next();
    return tmp;
  }

  T* operator->() {
    return m_object;
  }

  T const* operator->() const {
    return m_object;
  }

  T& operator*() const {
    return *m_object;
  }

  T& operator*() {
    return *m_object;
  }

  bool operator==(GameObjectIterator const& other) const
  {
    return m_it == other.m_it;
  }

  bool operator!=(GameObjectIterator const& other) const
  {
    return !(*this == other);
  }

private:
  void skip_to_next()
  {
    do
    {
      ++m_it;
      if (m_it == m_end)
      {
        break;
      }
      else
      {
        m_object = dynamic_cast<T*>(m_it->get());
      }
    }
    while (!m_object);
  }

private:
  Iterator m_it;
  Iterator m_end;
  T* m_object;
};

template<typename T>
class GameObjectRange
{
public:
  GameObjectRange(GameObjectManager const& manager) :
    m_manager(manager)
  {}

  GameObjectIterator<T> begin() const {
    return GameObjectIterator<T>(m_manager.get_objects().begin(), m_manager.get_objects().end());
  }

  GameObjectIterator<T> end() const {
    return GameObjectIterator<T>(m_manager.get_objects().end(), m_manager.get_objects().end());
  }

private:
  GameObjectManager const& m_manager;
};

#endif

/* EOF */

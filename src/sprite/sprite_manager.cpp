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

#include "sprite/sprite_manager.hpp"

#include "sprite/sprite.hpp"
#include "util/file_system.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"


SpriteManager::SpriteManager() :
  sprites()
{
}

SpritePtr
SpriteManager::create(std::string const& name)
{
  Sprites::iterator i = sprites.find(name);
  SpriteData* data;
  if (i == sprites.end()) {
    // try loading the spritefile
    data = load(name);
    if (data == nullptr) {
      std::stringstream msg;
      msg << "Sprite '" << name << "' not found.";
      throw std::runtime_error(msg.str());
    }
  } else {
    data = i->second.get();
  }

  return SpritePtr(new Sprite(*data));
}

SpriteData*
SpriteManager::load(std::string const& filename)
{
  ReaderDocument doc = [filename](){
    try {
      if (filename.ends_with(".sprite")) {
        return load_reader_document(filename);
      } else {
        std::stringstream text;
        text << "(supertux-sprite (actions (action "
             << "(name \"default\") "
             << "(images \"" << FileSystem::basename(filename) << "\"))))";
        return ReaderDocument::from_stream(text, prio::ErrorHandler::THROW, filename);
      }
    } catch(std::exception const& e) {
      std::ostringstream msg;
      msg << "Parse error when trying to load sprite '" << filename
      << "': " << e.what() << "\n";
      throw std::runtime_error(msg.str());
    }
  }();

  auto root = doc.get_root();

  if (root.get_name() != "supertux-sprite") {
    std::ostringstream msg;
    msg << "'" << filename << "' is not a supertux-sprite file";
    throw std::runtime_error(msg.str());
  } else {
    auto data = std::make_unique<SpriteData>(root.get_mapping());
    sprites[filename] = std::move(data);

    return sprites[filename].get();
  }
}

/* EOF */

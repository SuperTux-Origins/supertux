//  SuperTux
//  Copyright (C) 2020 A. Semphris <semphris@protonmail.com>
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

#include "object/custom_particle_system_file.hpp"

#include <algorithm>

#include "gui/menu_manager.hpp"
#include "util/reader.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"

CustomParticleSystemFile::CustomParticleSystemFile() :
  CustomParticleSystem(),
  m_filename()
{
}

CustomParticleSystemFile::CustomParticleSystemFile(const ReaderMapping& reader) :
  CustomParticleSystem(reader),
  m_filename()
{
  m_filename = reader.get("file", std::string("default.stcp"));
  std::replace(m_filename.begin(), m_filename.end(), '\\', '/');

  update_data();
  reinit_textures();
}

CustomParticleSystemFile::~CustomParticleSystemFile()
{
}

void
CustomParticleSystemFile::update_data()
{
  try
  {
    auto doc = ReaderDocument::from_file("particles/" + ((m_filename == "") ? "default.stcp" : m_filename));
    auto root = doc.get_root();
    auto mapping = root.get_mapping();

    if (root.get_name() != "supertux-custom-particle")
      throw std::runtime_error("file is not a supertux-custom-particle file.");

    set_props(CustomParticleSystem(mapping).get_props().get());
  }
  catch (std::exception& e)
  {
    log_warning << "Could not update custom particle from file (fallback to default settings): "
                    << e.what() << std::endl;
    set_props(CustomParticleSystem().get_props().get());
  }
}

/* EOF */

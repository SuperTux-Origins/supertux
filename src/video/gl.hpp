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

#ifndef HEADER_SUPERTUX_VIDEO_GL_HPP
#define HEADER_SUPERTUX_VIDEO_GL_HPP

#include <config.h>

#if defined(USE_OPENGLES2)
#  include <SDL_opengles2.h>
#else
#  include <GL/glew.h>
#  define GL_NONE_BIT 0
#endif

#ifdef USE_OPENGLES2
// These are required for OpenGL3.3Core, but not availabel en GLES2,
// simple no-op replacement looks prettier than #ifdef
inline void glGenVertexArrays(GLsizei n, GLuint *arrays) {}
inline void glDeleteVertexArrays(GLsizei n, GLuint *arrays) {}
inline void glBindVertexArray(GLuint vao) {}
#endif

#endif

/* EOF */

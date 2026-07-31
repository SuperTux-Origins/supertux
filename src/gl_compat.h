// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Unified OpenGL / OpenGL ES 2 header selection for SuperTux Milestone 1.
// Desktop immediate-mode path uses SDL_opengl.h; GLES2 uses Khronos GLES2.
// Isolate includes here so gameplay files do not grow version ifdefs.

#ifndef SUPERTUX_GL_COMPAT_H
#define SUPERTUX_GL_COMPAT_H

#ifndef NOOPENGL

#ifdef USE_GLES2
#  include <GLES2/gl2.h>
#else
#  include <SDL_opengl.h>
#endif

#endif /* !NOOPENGL */

#endif /* SUPERTUX_GL_COMPAT_H */

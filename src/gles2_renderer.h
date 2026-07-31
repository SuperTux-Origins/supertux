// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// GLES2 shader renderer for SuperTux Milestone 1.
// Replaces immediate-mode drawing (glBegin/glEnd) when USE_GLES2 is set.
// Only compiled/linked when ENABLE_GLES2=ON (see CMakeLists.txt).

#ifndef SUPERTUX_GLES2_RENDERER_H
#define SUPERTUX_GLES2_RENDERER_H

#ifdef USE_GLES2

#include "gl_compat.h"

/** Compile shaders, create program, set orthographic projection for ST_SCREEN. */
bool gles2_renderer_init(void);

/** Release GL program resources (safe if init was never called). */
void gles2_renderer_shutdown(void);

/** Recompute MVP for current drawable size / letterbox (call after resize). */
void gles2_renderer_set_viewport(int drawable_w, int drawable_h);

/** Apply an explicit letterbox rect (platform is source of truth for margins). */
void gles2_renderer_set_viewport_rect(int ox, int oy, int dw, int dh);

/** Full-drawable overlay pass in window pixels (Y-down). */
void gles2_renderer_set_overlay(int drawable_w, int drawable_h);

/** Textured axis-aligned quad in screen pixels; colour modulates texture (0–255). */
void gles2_draw_textured_quad(GLuint tex,
                              float x, float y, float w, float h,
                              float u0, float v0, float u1, float v1,
                              unsigned char r, unsigned char g,
                              unsigned char b, unsigned char a);

/** Solid-colour axis-aligned quad (0–255 channels). */
void gles2_draw_solid_quad(float x, float y, float w, float h,
                           unsigned char r, unsigned char g,
                           unsigned char b, unsigned char a);

/** Vertical gradient: top colour at y0, bottom colour at y1 (full width). */
void gles2_draw_gradient(float x, float y, float w, float h,
                         unsigned char r0, unsigned char g0, unsigned char b0,
                         unsigned char r1, unsigned char g1, unsigned char b1);

/** Single line segment (1px wide; uses a thin solid quad approximation). */
void gles2_draw_line(float x1, float y1, float x2, float y2,
                     unsigned char r, unsigned char g,
                     unsigned char b, unsigned char a);

#endif /* USE_GLES2 */

#endif /* SUPERTUX_GLES2_RENDERER_H */

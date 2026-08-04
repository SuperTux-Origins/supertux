// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// GLES2 shader renderer for SuperTux Milestone 1.
// Game content is rendered 1:1 into a 640×480 FBO, then scaled once into
// the window letterbox (matches the software backbuffer model and avoids
// per-tile scaling seams).

#ifndef SUPERTUX_GLES2_RENDERER_H
#define SUPERTUX_GLES2_RENDERER_H

#ifdef USE_GLES2

#include "gl_compat.h"

/** Compile shaders, create 640×480 FBO, bind it for drawing. */
bool gles2_renderer_init(void);

/** Release program + FBO (safe if init was never called). */
void gles2_renderer_shutdown(void);

/** Bind the offscreen 640×480 colour target (call after present / overlay). */
void gles2_renderer_bind_backbuffer(void);

/**
 * Scale the backbuffer into the window letterbox and leave the default
 * framebuffer bound (for overlays). ox/oy are top-down window coords;
 * drawable size is the full window in pixels.
 */
void gles2_renderer_present(int drawable_w, int drawable_h,
                            int ox, int oy, int dw, int dh);

/** MIN/MAG on the 640×480 FBO colour texture (whole-frame upscale). */
void gles2_renderer_set_frame_filter(bool linear);

/** Full-drawable overlay pass in window pixels (Y-down) on the default FB. */
void gles2_renderer_set_overlay(int drawable_w, int drawable_h);

/** Textured axis-aligned quad in logical screen pixels (0..640×480). */
void gles2_draw_textured_quad(GLuint tex,
                              float x, float y, float w, float h,
                              float u0, float v0, float u1, float v1,
                              unsigned char r, unsigned char g,
                              unsigned char b, unsigned char a);

/** Solid-colour axis-aligned quad (0–255 channels). */
void gles2_draw_solid_quad(float x, float y, float w, float h,
                           unsigned char r, unsigned char g,
                           unsigned char b, unsigned char a);

/** Vertical gradient: top colour at y, bottom at y+h. */
void gles2_draw_gradient(float x, float y, float w, float h,
                         unsigned char r0, unsigned char g0, unsigned char b0,
                         unsigned char r1, unsigned char g1, unsigned char b1);

/** Single line segment (thin solid quad). */
void gles2_draw_line(float x1, float y1, float x2, float y2,
                     unsigned char r, unsigned char g,
                     unsigned char b, unsigned char a);

/* Kept for call sites that still pass drawable size; no-ops viewport on FBO. */
void gles2_renderer_set_viewport(int drawable_w, int drawable_h);
void gles2_renderer_set_viewport_rect(int ox, int oy, int dw, int dh);

#endif /* USE_GLES2 */

#endif /* SUPERTUX_GLES2_RENDERER_H */

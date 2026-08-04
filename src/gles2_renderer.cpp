// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// GLES2 shader renderer — textured/solid quads into a 640×480 FBO, then
// one nearest-neighbour scale into the window letterbox.

#ifdef USE_GLES2

#include "gles2_renderer.h"
#include "platform.h"
#include "globals.h"
#include <cstdio>
#include <cstring>
#include <cmath>

namespace {

GLuint g_program = 0;
GLint  g_u_mvp = -1;
GLint  g_u_tex = -1;
GLint  g_u_use_tex = -1;
GLint  g_a_pos = -1;
GLint  g_a_uv = -1;
GLint  g_a_color = -1;
bool   g_ready = false;

GLuint g_fbo = 0;
GLuint g_fbo_tex = 0;
int    g_fbo_w = 0;
int    g_fbo_h = 0;

/* Column-major orthographic projection: maps (0,0)-(W,H) with Y-down. */
float g_mvp[16];

static void mat4_ortho(float* m, float left, float right, float bottom, float top)
{
  memset(m, 0, 16 * sizeof(float));
  m[0]  = 2.0f / (right - left);
  m[5]  = 2.0f / (top - bottom);
  m[10] = -1.0f;
  m[12] = -(right + left) / (right - left);
  m[13] = -(top + bottom) / (top - bottom);
  m[15] = 1.0f;
}

static GLuint compile_shader(GLenum type, const char* src)
{
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok)
    {
      char log[512];
      glGetShaderInfoLog(s, sizeof(log), NULL, log);
      fprintf(stderr, "GLES2 shader compile error (%s): %s\n",
              type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
      glDeleteShader(s);
      return 0;
    }
  return s;
}

static const char* kVS_ES =
  "#version 100\n"
  "attribute vec2 a_pos;\n"
  "attribute vec2 a_uv;\n"
  "attribute vec4 a_color;\n"
  "uniform mat4 u_mvp;\n"
  "varying vec2 v_uv;\n"
  "varying vec4 v_color;\n"
  "void main() {\n"
  "  gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);\n"
  "  v_uv = a_uv;\n"
  "  v_color = a_color;\n"
  "}\n";

static const char* kFS_ES =
  "#version 100\n"
  "precision mediump float;\n"
  "varying vec2 v_uv;\n"
  "varying vec4 v_color;\n"
  "uniform sampler2D u_tex;\n"
  "uniform float u_use_tex;\n"
  "void main() {\n"
  "  vec4 t = texture2D(u_tex, v_uv);\n"
  "  gl_FragColor = mix(v_color, t * v_color, u_use_tex);\n"
  "}\n";

static const char* kVS_GL =
  "#version 120\n"
  "attribute vec2 a_pos;\n"
  "attribute vec2 a_uv;\n"
  "attribute vec4 a_color;\n"
  "uniform mat4 u_mvp;\n"
  "varying vec2 v_uv;\n"
  "varying vec4 v_color;\n"
  "void main() {\n"
  "  gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);\n"
  "  v_uv = a_uv;\n"
  "  v_color = a_color;\n"
  "}\n";

static const char* kFS_GL =
  "#version 120\n"
  "varying vec2 v_uv;\n"
  "varying vec4 v_color;\n"
  "uniform sampler2D u_tex;\n"
  "uniform float u_use_tex;\n"
  "void main() {\n"
  "  vec4 t = texture2D(u_tex, v_uv);\n"
  "  gl_FragColor = mix(v_color, t * v_color, u_use_tex);\n"
  "}\n";

static bool context_looks_like_gles(void)
{
  const char* ver = (const char*)glGetString(GL_VERSION);
  if (!ver)
    return false;
  return strstr(ver, "OpenGL ES") != NULL || strstr(ver, "OpenGL-ES") != NULL;
}

static bool link_program(const char* vs_src, const char* fs_src)
{
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
  if (!vs || !fs)
    {
      if (vs) glDeleteShader(vs);
      if (fs) glDeleteShader(fs);
      return false;
    }

  g_program = glCreateProgram();
  glAttachShader(g_program, vs);
  glAttachShader(g_program, fs);
  glBindAttribLocation(g_program, 0, "a_pos");
  glBindAttribLocation(g_program, 1, "a_uv");
  glBindAttribLocation(g_program, 2, "a_color");
  glLinkProgram(g_program);
  glDeleteShader(vs);
  glDeleteShader(fs);

  GLint linked = 0;
  glGetProgramiv(g_program, GL_LINK_STATUS, &linked);
  if (!linked)
    {
      char log[512];
      glGetProgramInfoLog(g_program, sizeof(log), NULL, log);
      fprintf(stderr, "GLES2 program link error: %s\n", log);
      glDeleteProgram(g_program);
      g_program = 0;
      return false;
    }
  return true;
}

struct Vertex {
  float x, y;
  float u, v;
  float r, g, b, a;
};

static void draw_vertices(const Vertex* verts, int count, GLenum mode,
                          GLuint tex, bool use_tex)
{
  if (!g_ready)
    return;

  glUseProgram(g_program);
  glUniformMatrix4fv(g_u_mvp, 1, GL_FALSE, g_mvp);
  glUniform1f(g_u_use_tex, use_tex ? 1.0f : 0.0f);

  if (use_tex)
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex);
      glUniform1i(g_u_tex, 0);
    }
  else
    {
      glBindTexture(GL_TEXTURE_2D, 0);
    }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnableVertexAttribArray(g_a_pos);
  glEnableVertexAttribArray(g_a_uv);
  glEnableVertexAttribArray(g_a_color);

  const GLsizei stride = (GLsizei)sizeof(Vertex);
  glVertexAttribPointer(g_a_pos, 2, GL_FLOAT, GL_FALSE, stride, &verts[0].x);
  glVertexAttribPointer(g_a_uv, 2, GL_FLOAT, GL_FALSE, stride, &verts[0].u);
  glVertexAttribPointer(g_a_color, 4, GL_FLOAT, GL_FALSE, stride, &verts[0].r);

  glDrawArrays(mode, 0, count);

  glDisableVertexAttribArray(g_a_pos);
  glDisableVertexAttribArray(g_a_uv);
  glDisableVertexAttribArray(g_a_color);
  glDisable(GL_BLEND);
  glUseProgram(0);
}

static void destroy_fbo(void)
{
  if (g_fbo)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glDeleteFramebuffers(1, &g_fbo);
      g_fbo = 0;
    }
  if (g_fbo_tex)
    {
      glDeleteTextures(1, &g_fbo_tex);
      g_fbo_tex = 0;
    }
  g_fbo_w = g_fbo_h = 0;
}

static bool create_fbo(int w, int h)
{
  destroy_fbo();
  if (w < 1) w = ST_SCREEN_W;
  if (h < 1) h = ST_SCREEN_H;

  glGenTextures(1, &g_fbo_tex);
  glBindTexture(GL_TEXTURE_2D, g_fbo_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  {
    extern bool use_texture_filtering;
    GLint filt = use_texture_filtering ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filt);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filt);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &g_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, g_fbo_tex, 0);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      fprintf(stderr, "Error: GLES2 backbuffer FBO incomplete (0x%x)\n",
              (unsigned)status);
      destroy_fbo();
      return false;
    }

  g_fbo_w = w;
  g_fbo_h = h;
  glViewport(0, 0, w, h);
  mat4_ortho(g_mvp, 0.0f, (float)w, (float)h, 0.0f);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  return true;
}

} // namespace

bool gles2_renderer_init(void)
{
  gles2_renderer_shutdown();

  const char* gl_ver = (const char*)glGetString(GL_VERSION);
  const char* gl_ren = (const char*)glGetString(GL_RENDERER);
  const bool is_es = context_looks_like_gles();

  if (verbose_mode)
    {
      st_vlog("[video] GL context for shaders: ES=%s\n", is_es ? "yes" : "no");
      st_vlog("[video]   GL_VERSION=%s\n", gl_ver ? gl_ver : "(null)");
      st_vlog("[video]   GL_RENDERER=%s\n", gl_ren ? gl_ren : "(null)");
    }

  bool ok = false;
  if (is_es)
    {
      ok = link_program(kVS_ES, kFS_ES);
      if (!ok)
        fprintf(stderr, "Warning: GLSL ES 1.00 shaders failed; trying GLSL 1.20\n");
    }
  if (!ok)
    ok = link_program(kVS_GL, kFS_GL);
  if (!ok && !is_es)
    ok = link_program(kVS_ES, kFS_ES);
  if (!ok)
    {
      fprintf(stderr,
              "Error: could not compile/link shader program for GLES2 path\n"
              "  GL_VERSION=%s\n  GL_RENDERER=%s\n",
              gl_ver ? gl_ver : "(null)",
              gl_ren ? gl_ren : "(null)");
      return false;
    }

  g_u_mvp = glGetUniformLocation(g_program, "u_mvp");
  g_u_tex = glGetUniformLocation(g_program, "u_tex");
  g_u_use_tex = glGetUniformLocation(g_program, "u_use_tex");
  g_a_pos = glGetAttribLocation(g_program, "a_pos");
  g_a_uv = glGetAttribLocation(g_program, "a_uv");
  g_a_color = glGetAttribLocation(g_program, "a_color");

  if (!create_fbo(ST_SCREEN_W, ST_SCREEN_H))
    {
      glDeleteProgram(g_program);
      g_program = 0;
      return false;
    }

  g_ready = true;
  if (verbose_mode)
    st_vlog("[video] GLES2 renderer ready (FBO %dx%d + shader quads)\n",
            g_fbo_w, g_fbo_h);
  return true;
}

void gles2_renderer_shutdown(void)
{
  destroy_fbo();
  if (g_program)
    {
      glDeleteProgram(g_program);
      g_program = 0;
    }
  g_ready = false;
}

void gles2_renderer_bind_backbuffer(void)
{
  if (!g_ready || !g_fbo)
    return;
  glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
  glViewport(0, 0, g_fbo_w, g_fbo_h);
  mat4_ortho(g_mvp, 0.0f, (float)g_fbo_w, (float)g_fbo_h, 0.0f);
}

void gles2_renderer_set_frame_filter(bool linear)
{
  if (!g_fbo_tex)
    return;
  GLint filt = linear ? GL_LINEAR : GL_NEAREST;
  glBindTexture(GL_TEXTURE_2D, g_fbo_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filt);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filt);
}

void gles2_renderer_present(int drawable_w, int drawable_h,
                            int ox, int oy, int dw, int dh)
{
  if (!g_ready || !g_fbo_tex)
    return;
  if (drawable_w < 1) drawable_w = 1;
  if (drawable_h < 1) drawable_h = 1;
  if (dw < 1) dw = 1;
  if (dh < 1) dh = 1;

  /* Default framebuffer (window / canvas). */
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, drawable_w, drawable_h);
  mat4_ortho(g_mvp, 0.0f, (float)drawable_w, (float)drawable_h, 0.0f);

  glDisable(GL_BLEND);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  /* Letterbox rect is top-down (SDL); our ortho is also Y-down. */
  float x = (float)ox;
  float y = (float)oy;
  float w = (float)dw;
  float h = (float)dh;
  /* FBO texture is Y-up in GL; our FBO was drawn Y-down, so flip V. */
  Vertex verts[4] = {
    { x,     y,     0.f, 1.f, 1.f, 1.f, 1.f, 1.f },
    { x + w, y,     1.f, 1.f, 1.f, 1.f, 1.f, 1.f },
    { x,     y + h, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f },
    { x + w, y + h, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f },
  };
  draw_vertices(verts, 4, GL_TRIANGLE_STRIP, g_fbo_tex, true);
}

void gles2_renderer_set_viewport_rect(int /*ox*/, int /*oy*/, int /*dw*/, int /*dh*/)
{
  /* Drawing always targets the fixed FBO; letterbox is applied in present(). */
  gles2_renderer_bind_backbuffer();
}

void gles2_renderer_set_viewport(int /*drawable_w*/, int /*drawable_h*/)
{
  gles2_renderer_bind_backbuffer();
}

void gles2_renderer_set_overlay(int drawable_w, int drawable_h)
{
  if (drawable_w < 1) drawable_w = 1;
  if (drawable_h < 1) drawable_h = 1;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, drawable_w, drawable_h);
  mat4_ortho(g_mvp, 0.0f, (float)drawable_w, (float)drawable_h, 0.0f);
}

void gles2_draw_textured_quad(GLuint tex,
                              float x, float y, float w, float h,
                              float u0, float v0, float u1, float v1,
                              unsigned char r, unsigned char g,
                              unsigned char b, unsigned char a)
{
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f, af = a / 255.0f;
  Vertex verts[4] = {
    { x,     y,     u0, v0, rf, gf, bf, af },
    { x + w, y,     u1, v0, rf, gf, bf, af },
    { x,     y + h, u0, v1, rf, gf, bf, af },
    { x + w, y + h, u1, v1, rf, gf, bf, af },
  };
  draw_vertices(verts, 4, GL_TRIANGLE_STRIP, tex, true);
}

void gles2_draw_solid_quad(float x, float y, float w, float h,
                           unsigned char r, unsigned char g,
                           unsigned char b, unsigned char a)
{
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f, af = a / 255.0f;
  Vertex verts[4] = {
    { x,     y,     0, 0, rf, gf, bf, af },
    { x + w, y,     0, 0, rf, gf, bf, af },
    { x,     y + h, 0, 0, rf, gf, bf, af },
    { x + w, y + h, 0, 0, rf, gf, bf, af },
  };
  draw_vertices(verts, 4, GL_TRIANGLE_STRIP, 0, false);
}

void gles2_draw_gradient(float x, float y, float w, float h,
                         unsigned char r0, unsigned char g0, unsigned char b0,
                         unsigned char r1, unsigned char g1, unsigned char b1)
{
  float r0f = r0 / 255.0f, g0f = g0 / 255.0f, b0f = b0 / 255.0f;
  float r1f = r1 / 255.0f, g1f = g1 / 255.0f, b1f = b1 / 255.0f;
  Vertex verts[4] = {
    { x,     y,     0, 0, r0f, g0f, b0f, 1.f },
    { x + w, y,     0, 0, r0f, g0f, b0f, 1.f },
    { x,     y + h, 0, 0, r1f, g1f, b1f, 1.f },
    { x + w, y + h, 0, 0, r1f, g1f, b1f, 1.f },
  };
  draw_vertices(verts, 4, GL_TRIANGLE_STRIP, 0, false);
}

void gles2_draw_line(float x1, float y1, float x2, float y2,
                     unsigned char r, unsigned char g,
                     unsigned char b, unsigned char a)
{
  float dx = x2 - x1, dy = y2 - y1;
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 0.001f)
    {
      gles2_draw_solid_quad(x1, y1, 1.0f, 1.0f, r, g, b, a);
      return;
    }
  /* Axis-aligned approximation for short HUD lines. */
  if (fabsf(dx) >= fabsf(dy))
    gles2_draw_solid_quad(x1 < x2 ? x1 : x2, y1, len, 1.0f, r, g, b, a);
  else
    gles2_draw_solid_quad(x1, y1 < y2 ? y1 : y2, 1.0f, len, r, g, b, a);
}

#endif /* USE_GLES2 */

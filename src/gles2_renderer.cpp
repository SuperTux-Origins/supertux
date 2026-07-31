// GLES2 shader renderer — textured and solid quads for SuperTux Milestone 1.

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

/* GLSL ES 1.00 (#version 100) — required by many GLES2 compilers; without
   it Mesa reports "syntax error, unexpected NEW_IDENTIFIER" on attribute. */
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

/* Desktop GLSL 1.20 fallback if the context is not actually ES (some
   drivers ignore PROFILE_ES and hand back a compatibility context). */
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
  /* Typical strings: "OpenGL ES 2.0 ...", "OpenGL ES 3.1 ..." */
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

  /* Prefer ES 1.00 sources on an ES context; fall back to GLSL 1.20 if the
     driver handed us a desktop compatibility context despite PROFILE_ES. */
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
    {
      /* Last resort: ES sources on a non-ES context (some EGL stacks). */
      ok = link_program(kVS_ES, kFS_ES);
    }
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

  mat4_ortho(g_mvp, 0.0f, (float)ST_SCREEN_W, (float)ST_SCREEN_H, 0.0f);
  g_ready = true;
  if (verbose_mode)
    st_vlog("[video] GLES2 renderer ready (shader textured/solid quads)\n");
  return true;
}

void gles2_renderer_shutdown(void)
{
  if (g_program)
    {
      glDeleteProgram(g_program);
      g_program = 0;
    }
  g_ready = false;
}

void gles2_renderer_set_viewport_rect(int ox, int oy, int dw, int dh)
{
  if (dw < 1) dw = 1;
  if (dh < 1) dh = 1;
  glViewport(ox, oy, dw, dh);
  mat4_ortho(g_mvp, 0.0f, (float)ST_SCREEN_W, (float)ST_SCREEN_H, 0.0f);
}

void gles2_renderer_set_viewport(int drawable_w, int drawable_h)
{
  /* Fallback when platform margins are not applied: classic centered fit. */
  float sx = (float)drawable_w / (float)ST_SCREEN_W;
  float sy = (float)drawable_h / (float)ST_SCREEN_H;
  float scale = (sx < sy) ? sx : sy;
  int dw = (int)(ST_SCREEN_W * scale + 0.5f);
  int dh = (int)(ST_SCREEN_H * scale + 0.5f);
  if (dw < 1) dw = 1;
  if (dh < 1) dh = 1;
  gles2_renderer_set_viewport_rect((drawable_w - dw) / 2,
                                   (drawable_h - dh) / 2, dw, dh);
}

void gles2_renderer_set_overlay(int drawable_w, int drawable_h)
{
  if (drawable_w < 1) drawable_w = 1;
  if (drawable_h < 1) drawable_h = 1;
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
  float tR = r0 / 255.0f, tG = g0 / 255.0f, tB = b0 / 255.0f;
  float bR = r1 / 255.0f, bG = g1 / 255.0f, bB = b1 / 255.0f;
  Vertex verts[4] = {
    { x,     y,     0, 0, tR, tG, tB, 1.0f },
    { x + w, y,     0, 0, tR, tG, tB, 1.0f },
    { x,     y + h, 0, 0, bR, bG, bB, 1.0f },
    { x + w, y + h, 0, 0, bR, bG, bB, 1.0f },
  };
  draw_vertices(verts, 4, GL_TRIANGLE_STRIP, 0, false);
}

void gles2_draw_line(float x1, float y1, float x2, float y2,
                     unsigned char r, unsigned char g,
                     unsigned char b, unsigned char a)
{
  /* Approximate a 1px line as a thin quad so we stay on the triangle path. */
  float dx = x2 - x1;
  float dy = y2 - y1;
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 1e-4f)
    {
      gles2_draw_solid_quad(x1, y1, 1.0f, 1.0f, r, g, b, a);
      return;
    }
  float nx = -dy / len * 0.5f;
  float ny =  dx / len * 0.5f;
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f, af = a / 255.0f;
  Vertex verts[4] = {
    { x1 + nx, y1 + ny, 0, 0, rf, gf, bf, af },
    { x1 - nx, y1 - ny, 0, 0, rf, gf, bf, af },
    { x2 + nx, y2 + ny, 0, 0, rf, gf, bf, af },
    { x2 - nx, y2 - ny, 0, 0, rf, gf, bf, af },
  };
  draw_vertices(verts, 4, GL_TRIANGLE_STRIP, 0, false);
}

#endif /* USE_GLES2 */

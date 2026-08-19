/* SDL_image replacement for offline Emscripten via stb_image (PNG/JPEG/…). */
#include "SDL_image.h"
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"

static SDL_version g_img_version = {2, 0, 0};

const SDL_version *IMG_Linked_Version(void)
{
  return &g_img_version;
}

int IMG_Init(int flags)
{
  (void)flags;
  return flags;
}

void IMG_Quit(void)
{
}

const char *IMG_GetError(void)
{
  return SDL_GetError();
}

static SDL_Surface *surface_from_rgba(unsigned char *data, int w, int h)
{
  if (!data)
    return NULL;
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
  if (!s)
  {
    stbi_image_free(data);
    return NULL;
  }
  memcpy(s->pixels, data, (size_t)w * (size_t)h * 4);
  stbi_image_free(data);
  return s;
}

SDL_Surface *IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, const char *type)
{
  (void)type;
  if (!src)
  {
    SDL_SetError("IMG_LoadTyped_RW: null RWops");
    return NULL;
  }
  Sint64 size = SDL_RWsize(src);
  if (size <= 0)
  {
    if (freesrc)
      SDL_RWclose(src);
    SDL_SetError("IMG_LoadTyped_RW: empty or unknown size");
    return NULL;
  }
  unsigned char *buf = (unsigned char *)malloc((size_t)size);
  if (!buf)
  {
    if (freesrc)
      SDL_RWclose(src);
    SDL_SetError("IMG_LoadTyped_RW: OOM");
    return NULL;
  }
  if (SDL_RWread(src, buf, 1, (size_t)size) != (size_t)size)
  {
    free(buf);
    if (freesrc)
      SDL_RWclose(src);
    SDL_SetError("IMG_LoadTyped_RW: short read");
    return NULL;
  }
  if (freesrc)
    SDL_RWclose(src);

  int w = 0, h = 0, n = 0;
  unsigned char *rgba = stbi_load_from_memory(buf, (int)size, &w, &h, &n, 4);
  free(buf);
  if (!rgba)
  {
    SDL_SetError("stbi: %s", stbi_failure_reason() ? stbi_failure_reason() : "decode failed");
    return NULL;
  }
  return surface_from_rgba(rgba, w, h);
}

SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc)
{
  return IMG_LoadTyped_RW(src, freesrc, NULL);
}

SDL_Surface *IMG_Load(const char *file)
{
  SDL_RWops *rw = SDL_RWFromFile(file, "rb");
  if (!rw)
    return NULL;
  return IMG_Load_RW(rw, 1);
}

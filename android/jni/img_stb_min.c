/* Minimal SDL_image API for Android using upstream stb_image.h */
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

static char img_error[256];

const char *IMG_GetError(void)
{
  const char *sdl = SDL_GetError();
  if (sdl && sdl[0])
    return sdl;
  return img_error[0] ? img_error : "";
}

static void set_err(const char *msg)
{
  if (msg && msg[0])
    SDL_SetError("%s", msg);
  snprintf(img_error, sizeof(img_error), "%s", msg ? msg : "");
}

int IMG_Init(int flags)
{
  img_error[0] = '\0';
  return flags;
}

void IMG_Quit(void)
{
  img_error[0] = '\0';
}

SDL_Surface *IMG_Load(const char *file)
{
  int w = 0, h = 0, n = 0;
  unsigned char *data;
  SDL_Surface *surf;

  img_error[0] = '\0';
  if (!file) {
    set_err("IMG_Load: NULL filename");
    return NULL;
  }

  data = stbi_load(file, &w, &h, &n, 4);
  if (!data) {
    const char *why = stbi_failure_reason();
    set_err(why ? why : "stbi_load failed");
    return NULL;
  }

  surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
  if (!surf) {
    stbi_image_free(data);
    set_err(SDL_GetError());
    return NULL;
  }
  memcpy(surf->pixels, data, (size_t)w * (size_t)h * 4u);
  stbi_image_free(data);
  return surf;
}

/* Minimal SDL2_image-compatible loader for Android using stb_image.
 * Implements the subset SuperTux calls: IMG_Init, IMG_Quit, IMG_Load, IMG_GetError.
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
/* Prefer the copy shipped inside SDL2_image when present. */
#if defined(__has_include)
#  if __has_include("SDL2_image/src/stb_image.h")
#    include "SDL2_image/src/stb_image.h"
#  elif __has_include("stb_image.h")
#    include "stb_image.h"
#  else
#    error "stb_image.h not found"
#  endif
#else
#  include "SDL2_image/src/stb_image.h"
#endif

static char img_error[256];

const char *IMG_GetError(void)
{
  return img_error[0] ? img_error : SDL_GetError();
}

static void set_err(const char *msg)
{
  snprintf(img_error, sizeof(img_error), "%s", msg ? msg : "");
}

int IMG_Init(int flags)
{
  (void)flags;
  img_error[0] = '\0';
  return flags; /* claim whatever was requested */
}

void IMG_Quit(void)
{
  img_error[0] = '\0';
}

SDL_Surface *IMG_Load(const char *file)
{
  int w = 0, h = 0, n = 0;
  unsigned char *data;

  img_error[0] = '\0';
  if (!file) {
    set_err("IMG_Load: NULL filename");
    return NULL;
  }

  data = stbi_load(file, &w, &h, &n, 4);
  if (!data) {
    set_err(stbi_failure_reason() ? stbi_failure_reason() : "stbi_load failed");
    return NULL;
  }

  {
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(
        data, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
    if (!surf) {
      stbi_image_free(data);
      set_err(SDL_GetError());
      return NULL;
    }
    /* SDL_Surface does not free the pixel pointer on FreeSurface when
       created with CreateRGBSurfaceWithFormatFrom — own a copy instead. */
    {
      SDL_Surface *copy = SDL_ConvertSurface(surf, surf->format, 0);
      SDL_FreeSurface(surf);
      stbi_image_free(data);
      if (!copy) {
        set_err(SDL_GetError());
        return NULL;
      }
      return copy;
    }
  }
}

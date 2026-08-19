/* Offline SDL_image stub for Emscripten (no -sUSE_SDL_IMAGE port).
 * SuperTux loads surfaces via IMG_Load_RW; real decoding can be wired to
 * stb_image later. Returning NULL keeps the game compiling offline.
 */
#include "SDL_image.h"
#include <stddef.h>

static SDL_version g_img_version = {2, 0, 0};

const SDL_version *IMG_Linked_Version(void)
{
  return &g_img_version;
}

int IMG_Init(int flags)
{
  (void)flags;
  return 0;
}

void IMG_Quit(void)
{
}

SDL_Surface *IMG_Load(const char *file)
{
  (void)file;
  return NULL;
}

SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc)
{
  if (src && freesrc)
    SDL_RWclose(src);
  return NULL;
}

SDL_Surface *IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, const char *type)
{
  (void)type;
  return IMG_Load_RW(src, freesrc);
}

const char *IMG_GetError(void)
{
  return "SDL_image stub: image load not implemented offline";
}

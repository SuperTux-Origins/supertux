/* Offline SDL_ttf stub implementation for Emscripten. */
#include "SDL_ttf_stub.h"
#include <stddef.h>

int TTF_Init(void) { return 0; }
void TTF_Quit(void) {}

TTF_Font *TTF_OpenFont(const char *file, int ptsize)
{
  (void)file; (void)ptsize;
  return NULL;
}

TTF_Font *TTF_OpenFontRW(SDL_RWops *src, int freesrc, int ptsize)
{
  (void)ptsize;
  if (src && freesrc)
    SDL_RWclose(src);
  return NULL;
}

void TTF_CloseFont(TTF_Font *font) { (void)font; }

int TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h)
{
  (void)font; (void)text;
  if (w) *w = 0;
  if (h) *h = 0;
  return -1;
}

int TTF_FontHeight(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontAscent(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontDescent(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontLineSkip(const TTF_Font *font) { (void)font; return 0; }

SDL_Surface *TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg)
{
  (void)font; (void)text; (void)fg;
  return NULL;
}

SDL_Surface *TTF_RenderUTF8_Solid(TTF_Font *font, const char *text, SDL_Color fg)
{
  (void)font; (void)text; (void)fg;
  return NULL;
}

SDL_Surface *TTF_RenderUTF8_Shaded(TTF_Font *font, const char *text, SDL_Color fg, SDL_Color bg)
{
  (void)font; (void)text; (void)fg; (void)bg;
  return NULL;
}


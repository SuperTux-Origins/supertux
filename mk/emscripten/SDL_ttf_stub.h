/* Offline SDL_ttf stub for Emscripten (no -sUSE_FREETYPE / -sUSE_SDL port).
 * Real glyph rendering can be wired later; returns NULL / zero so the game links.
 */
#ifndef SUPERTUX_EMSCRIPTEN_SDL_TTF_H
#define SUPERTUX_EMSCRIPTEN_SDL_TTF_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TTF_Font TTF_Font;

int TTF_Init(void);
void TTF_Quit(void);
TTF_Font *TTF_OpenFont(const char *file, int ptsize);
TTF_Font *TTF_OpenFontRW(SDL_RWops *src, int freesrc, int ptsize);
void TTF_CloseFont(TTF_Font *font);
int TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h);
int TTF_FontHeight(const TTF_Font *font);
int TTF_FontAscent(const TTF_Font *font);
int TTF_FontDescent(const TTF_Font *font);
int TTF_FontLineSkip(const TTF_Font *font);
SDL_Surface *TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg);
SDL_Surface *TTF_RenderUTF8_Solid(TTF_Font *font, const char *text, SDL_Color fg);
SDL_Surface *TTF_RenderUTF8_Shaded(TTF_Font *font, const char *text, SDL_Color fg, SDL_Color bg);
/* Match real SDL_ttf: these are macros, not separate symbols. */
#define TTF_GetError   SDL_GetError
#define TTF_SetError   SDL_SetError

#ifdef __cplusplus
}
#endif

#endif /* SUPERTUX_EMSCRIPTEN_SDL_TTF_H */

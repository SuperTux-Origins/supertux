/* Fallback SDL_ttf when FreeType is not staged for this APK build. */
#include "SDL_ttf.h"

int TTF_Init(void) { return 0; }
void TTF_Quit(void) {}
const SDL_version *TTF_Linked_Version(void) {
  static SDL_version v = {2, 0, 0};
  return &v;
}
TTF_Font *TTF_OpenFont(const char *file, int ptsize) {
  (void)file; (void)ptsize; return NULL;
}
TTF_Font *TTF_OpenFontIndex(const char *file, int ptsize, long index) {
  (void)file; (void)ptsize; (void)index; return NULL;
}
TTF_Font *TTF_OpenFontRW(SDL_RWops *src, int freesrc, int ptsize) {
  if (src && freesrc) SDL_RWclose(src);
  (void)ptsize; return NULL;
}
void TTF_CloseFont(TTF_Font *font) { (void)font; }
int TTF_GetFontStyle(const TTF_Font *font) { (void)font; return 0; }
void TTF_SetFontStyle(TTF_Font *font, int style) { (void)font; (void)style; }
int TTF_FontHeight(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontAscent(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontDescent(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontLineSkip(const TTF_Font *font) { (void)font; return 0; }
int TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h) {
  (void)font; (void)text; if (w) *w = 0; if (h) *h = 0; return -1;
}
SDL_Surface *TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg) {
  (void)font; (void)text; (void)fg; return NULL;
}
SDL_Surface *TTF_RenderUTF8_Solid(TTF_Font *font, const char *text, SDL_Color fg) {
  (void)font; (void)text; (void)fg; return NULL;
}
const char *TTF_GetError(void) { return "SDL_ttf stub (FreeType not linked)"; }

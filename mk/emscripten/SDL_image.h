/* Minimal SDL_image for offline Emscripten (no USE_SDL_IMAGE port / zlib fetch). */
#ifndef SUPERTUX_EMSCRIPTEN_SDL_IMAGE_H
#define SUPERTUX_EMSCRIPTEN_SDL_IMAGE_H
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SDL_version SDL_version;
const SDL_version *IMG_Linked_Version(void);
int IMG_Init(int flags);
void IMG_Quit(void);
SDL_Surface *IMG_Load(const char *file);
SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc);
SDL_Surface *IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, const char *type);
const char *IMG_GetError(void);
#define IMG_INIT_JPG 0x1
#define IMG_INIT_PNG 0x2
#define IMG_INIT_TIF 0x4
#define IMG_INIT_WEBP 0x8
#ifdef __cplusplus
}
#endif
#endif

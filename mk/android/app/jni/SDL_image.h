#ifndef SDL_IMAGE_H_
#define SDL_IMAGE_H_
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
extern const SDL_version *IMG_Linked_Version(void);
extern int IMG_Init(int flags);
extern void IMG_Quit(void);
extern SDL_Surface *IMG_Load(const char *file);
extern SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc);
extern SDL_Surface *IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, const char *type);
extern const char *IMG_GetError(void);
#define IMG_SetError SDL_SetError
/* flags unused on Android stb path */
#define IMG_INIT_JPG 0x1
#define IMG_INIT_PNG 0x2
#define IMG_INIT_TIF 0x4
#define IMG_INIT_WEBP 0x8
#ifdef __cplusplus
}
#endif
#endif

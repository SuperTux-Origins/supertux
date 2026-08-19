/* Temporary Android entry until full SuperTux sources are linked via
 * supertux_sources.mk. Logs each init step so logcat shows progress. */
#include <SDL.h>
#include <android/log.h>
#include <unistd.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "supertux", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "supertux", __VA_ARGS__)

extern "C" int SDL_main(int argc, char* argv[]) {
  LOGI("SDL_main enter argc=%d", argc);
  for (int i = 0; i < argc; ++i) {
    LOGI("  argv[%d]=%s", i, argv[i] ? argv[i] : "(null)");
  }

  LOGI("SDL_Init(SDL_INIT_VIDEO|AUDIO|JOYSTICK|GAMECONTROLLER)...");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK |
               SDL_INIT_GAMECONTROLLER) != 0) {
    LOGE("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }
  LOGI("SDL_Init ok; SDL_GetRevision=%s", SDL_GetRevision());

  SDL_DisplayMode mode;
  if (SDL_GetCurrentDisplayMode(0, &mode) == 0) {
    LOGI("display0: %dx%d @%dHz format=%u", mode.w, mode.h, mode.refresh_rate,
         (unsigned)mode.format);
  } else {
    LOGE("SDL_GetCurrentDisplayMode: %s", SDL_GetError());
  }

  LOGI("SDL_CreateWindow 640x480...");
  SDL_Window* win = SDL_CreateWindow(
      "SuperTux Origins (Android placeholder)",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      640, 480,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (!win) {
    LOGE("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 2;
  }
  LOGI("window created id=%u", (unsigned)SDL_GetWindowID(win));

  SDL_GLContext gl = SDL_GL_CreateContext(win);
  if (!gl) {
    LOGE("SDL_GL_CreateContext failed: %s (continuing without GL)", SDL_GetError());
  } else {
    LOGI("GL context ok");
  }

  LOGI("placeholder: keeping window open ~3s then exit (full game not linked yet)");
  Uint32 start = SDL_GetTicks();
  while (SDL_GetTicks() - start < 3000) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        LOGI("SDL_QUIT received");
        goto done;
      }
    }
    if (gl) {
      SDL_GL_SwapWindow(win);
    }
    SDL_Delay(16);
  }
  LOGI("placeholder timeout — exiting (wire SUPERTUX_SOURCES for real game)");

done:
  if (gl) SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(win);
  SDL_Quit();
  LOGI("SDL_main leave");
  return 0;
}

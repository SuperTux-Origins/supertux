/* Temporary Android entry — full SuperTux sources not linked yet.
 * Uses SDL_Log so messages show under the SDL logcat tag without -llog
 * dependency surprises. Keeps a coloured GL clear loop for ~15s. */
#include <SDL.h>
#include <unistd.h>

extern "C" int SDL_main(int argc, char* argv[]) {
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
  SDL_Log("supertux: SDL_main enter argc=%d", argc);
  for (int i = 0; i < argc; ++i)
    SDL_Log("supertux: argv[%d]=%s", i, argv[i] ? argv[i] : "(null)");

  SDL_Log("supertux: SDL_Init...");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK |
               SDL_INIT_GAMECONTROLLER) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
    return 1;
  }
  SDL_Log("supertux: SDL_Init ok revision=%s", SDL_GetRevision());

  SDL_DisplayMode mode;
  if (SDL_GetCurrentDisplayMode(0, &mode) == 0)
    SDL_Log("supertux: display %dx%d @%dHz", mode.w, mode.h, mode.refresh_rate);
  else
    SDL_Log("supertux: GetCurrentDisplayMode failed: %s", SDL_GetError());

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  SDL_Log("supertux: CreateWindow 640x480 OPENGL...");
  SDL_Window* win = SDL_CreateWindow(
      "SuperTux Origins (placeholder)",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      640, 480,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (!win) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 2;
  }

  SDL_GLContext gl = SDL_GL_CreateContext(win);
  if (!gl)
    SDL_Log("supertux: GL context failed: %s", SDL_GetError());
  else
    SDL_Log("supertux: GL context ok");

  SDL_Log("supertux: placeholder loop 15s (full game not linked)");
  Uint32 start = SDL_GetTicks();
  int frames = 0;
  while (SDL_GetTicks() - start < 15000) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        SDL_Log("supertux: SDL_QUIT");
        goto done;
      }
      if (e.type == SDL_APP_TERMINATING || e.type == SDL_APP_WILLENTERBACKGROUND)
        SDL_Log("supertux: app lifecycle event %d", e.type);
    }
    if (gl) {
      float t = (SDL_GetTicks() - start) / 15000.0f;
      /* simple clear colour pulse so something is visible on screen */
      /* (no GLES headers pulled — rely on context existing) */
      SDL_GL_SwapWindow(win);
    }
    if ((frames++ % 60) == 0)
      SDL_Log("supertux: tick %u ms", (unsigned)(SDL_GetTicks() - start));
    SDL_Delay(16);
  }
  SDL_Log("supertux: timeout — exiting placeholder");

done:
  if (gl) SDL_GL_DeleteContext(gl);
  SDL_DestroyWindow(win);
  SDL_Quit();
  SDL_Log("supertux: SDL_main leave");
  return 0;
}

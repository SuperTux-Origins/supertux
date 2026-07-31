#include "game_file.h"
#include "globals.h"

#include <string.h>

std::string
game_file_relative(const std::string& path)
{
  std::string p = path;

  if (!datadir.empty()
      && p.size() >= datadir.size()
      && p.compare(0, datadir.size(), datadir) == 0)
    {
      p = p.substr(datadir.size());
    }

  while (!p.empty() && (p[0] == '/' || p[0] == '\\'))
    p.erase(0, 1);

  if (p.size() >= 2 && p[0] == '.' && (p[1] == '/' || p[1] == '\\'))
    p.erase(0, 2);

  return p;
}

SDL_RWops*
open_game_file(const std::string& path)
{
  if (path.empty())
    return 0;

  /* Prefer real filesystem (desktop install, user mods, writable paths). */
  SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb");
  if (rw)
    return rw;

  /* Fall back to datadir-relative path — on Android SDL opens APK assets. */
  std::string rel = game_file_relative(path);
  if (!rel.empty() && rel != path)
    {
      rw = SDL_RWFromFile(rel.c_str(), "rb");
      if (rw)
        return rw;
    }

  return 0;
}

bool
game_file_exists(const std::string& path)
{
  SDL_RWops* rw = open_game_file(path);
  if (!rw)
    return false;
  SDL_RWclose(rw);
  return true;
}

bool
game_file_read(const std::string& path, std::vector<char>& out)
{
  out.clear();
  SDL_RWops* rw = open_game_file(path);
  if (!rw)
    return false;

  /* Chunked read works on SDL1 and SDL2 (no SDL_RWsize required). */
  char buf[4096];
  size_t n;
  while ((n = SDL_RWread(rw, buf, 1, sizeof(buf))) > 0)
    out.insert(out.end(), buf, buf + n);

  SDL_RWclose(rw);
  return true;
}

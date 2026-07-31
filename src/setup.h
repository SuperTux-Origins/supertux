// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_SETUP_H
#define SUPERTUX_SETUP_H

#include "menu.h"
#ifndef NOSOUND
#include "sound.h"
#endif
#include "type.h"

int faccessible(const char *filename);
int fcreatedir(const char* relative_dir);
int fwriteable(const char *filename);
FILE * opendata(const char * filename, const char * mode);
string_list_type dsubdirs(const char *rel_path, const char* expected_file);
string_list_type dfiles(const char *rel_path, const char* glob, const char* exception_str);
void free_strings(char **strings, int num);
void st_directory_setup(void);
void st_sdl_init(void);
void st_general_setup(void);
void st_general_free();
void st_video_setup_sdl(void);
void st_video_setup_gl(void);
void st_video_setup(void);
void st_audio_setup(void);
void st_joystick_setup(void);
void st_shutdown(void);
void st_menu(void);
void st_abort(const std::string& reason, const std::string& details);
/** When --verbose: print render path and subsystem init/skip summary to stderr. */
void st_print_init_status(void);
void process_options_menu(void);

/** Return true if the gameloop() was entered, false otherwise */
bool process_load_game_menu();

void update_load_save_game_menu(Menu* pmenu);
void parse_path_args(int argc, char * argv[]);
void parseargs(int argc, char * argv[]);

#endif /*SUPERTUX_SETUP_H*/


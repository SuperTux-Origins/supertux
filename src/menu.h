// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_MENU_H
#define SUPERTUX_MENU_H

#include "platform_config.h"
#include <vector>
#include "texture.h"
#include "timer.h"
#include "type.h"
#include "mousecursor.h"

/* IDs for menus */

enum MainMenuIDs {
  MNID_STARTGAME,
  MNID_CONTRIB,
  MNID_OPTIONMENU,
  MNID_LEVELEDITOR,
  MNID_CREDITS,
  MNID_QUITMAINMENU
  };

enum OptionsMenuIDs {
  MNID_OPENGL,
  MNID_FULLSCREEN,
  MNID_TEXTUREFILTER,
  MNID_SOUND,
  MNID_MUSIC,
  MNID_SHOWFPS,
  MNID_SHOWMOUSE,
  MNID_TOUCH_SCHEME,
  /* SDL2 GameController device picker (options_gamepad_device_menu). */
  MNID_GAMEPAD_NONE = 900,          /* keyboard only / close pad */
  MNID_GAMEPAD_DEVICE_BASE = 901    /* + device index 0..N-1 */
  };

enum GameMenuIDs {
  MNID_CONTINUE,
  MNID_ABORTLEVEL
  };

enum WorldMapMenuIDs {
  MNID_RETURNWORLDMAP,
  MNID_QUITWORLDMAP
  };

enum LevelEditorMainMenuIDs {
  MNID_RETURNLEVELEDITOR,
  MNID_SUBSETSETTINGS,
  MNID_QUITLEVELEDITOR
  };
  
enum LevelEditorSubsetSettingsIDs {
  MNID_SUBSETTITLE,
  MNID_SUBSETDESCRIPTION,
  MNID_SUBSETSAVECHANGES
  };
  
enum LevelEditorSubsetNewIDs {
 MNID_SUBSETNAME,
 MNID_CREATESUBSET
};

enum LevelEditorSettingsMenuIDs {
  MNID_NAME,
  MNID_AUTHOR,
  MNID_SONG,
  MNID_BGIMG,
  MNID_PARTICLE,
  MNID_LENGTH,
  MNID_TIME,
  MNID_GRAVITY,
  MNID_BGSPEED,
  MNID_TopRed,
  MNID_TopGreen,
  MNID_TopBlue,
  MNID_BottomRed,
  MNID_BottomGreen,
  MNID_BottomBlue,
  MNID_APPLY
  };

bool confirm_dialog(std::string text);
/** Split form for a future non-blocking pump (confirm_dialog still blocks).
 *  text: optional header (empty = no header line).
 *  confirm_label / cancel_label: action buttons (defaults Yes / No). */
void confirm_dialog_begin(const std::string& text,
                          const std::string& confirm_label = "Yes",
                          const std::string& cancel_label = "No");
bool confirm_dialog_frame(void);  /* true while dialog is open */
bool confirm_dialog_result(void); /* valid after frame returns false */

/* Kinds of menu items */
enum MenuItemKind {
  MN_ACTION,
  MN_GOTO,
  MN_TOGGLE,
  MN_BACK,
  MN_DEACTIVE,
  MN_TEXTFIELD,
  MN_NUMFIELD,
  MN_CONTROLFIELD,
  MN_STRINGSELECT,
  MN_LABEL,
  MN_HL, /* horizontal line */
};

class Menu;

class MenuItem
{
public:
  MenuItemKind kind;
  int toggled;
  char *text;
  char *input;
  int *int_p;   // used for setting keys (can be used for more stuff...)
  int id;   // item id
  string_list_type* list;
  Menu* target_menu;

  void change_text (const char *text);
  void change_input(const char *text);

  static MenuItem* create(MenuItemKind kind, const char *text, int init_toggle, Menu* target_menu, int id, int* int_p);

  std::string get_input_with_symbol(bool active_item);   // returns the text with an input symbol
private:
  bool input_flickering;
  Timer input_flickering_timer;
};

class Menu
{
private:  
  static std::vector<Menu*> last_menus;
  static Menu* current_;

  static void push_current(Menu* pmenu);
  static void pop_current();

public:
  /** Set the current menu, if pmenu is NULL, hide the current menu */
  static void set_current(Menu* pmenu);

  /** Return the current active menu or NULL if none is active */
  static Menu* current() { return current_; }

private:
  /* Action done on the menu */
  enum MenuAction {
    MENU_ACTION_NONE = -1,
    MENU_ACTION_UP,
    MENU_ACTION_DOWN,
    MENU_ACTION_LEFT,
    MENU_ACTION_RIGHT,
    MENU_ACTION_HIT,
    MENU_ACTION_INPUT,
    MENU_ACTION_REMOVE
  };

  /** Number of the item that got 'hit' (ie. pressed) in the last
      event()/action() call, -1 if none */
  int hit_item;

  // position of the menu (ie. center of the menu, not top/left)
  int pos_x;
  int pos_y;

  /** input event for the menu (up, down, left, right, etc.) */
  MenuAction menuaction;

  /* input implementation variables */
  int delete_character;
  char mn_input_char;
  
public:
  Timer effect;
  int arrange_left;
  int active_item;
  /** Index of MN_CONTROLFIELD waiting for a key/button, or -1 when idle.
      Navigation works while idle; Enter/HIT starts bind; Escape cancels. */
  int control_bind_item;

  std::vector<MenuItem> item;

  Menu();
  ~Menu();

  void additem(MenuItem* pmenu_item);
  void additem(MenuItemKind kind, const std::string& text, int init_toggle, Menu* target_menu, int id = -1, int *int_p = NULL);
  
  void  action ();
  
  /** Remove all entries from the menu */
  void clear();

  /** Return the index of the menu item that was 'hit' (ie. the user
      clicked on it) in the last event() call */
  int  check  ();

  MenuItem& get_item(int index) { return item[index]; }
  MenuItem& get_item_by_id(int id);

  int get_active_item_id();

  bool isToggled(int id);

  void get_controlfield_key_into_input(MenuItem *item);

  void draw   ();
  void draw_item(int index, int menu_width, int menu_height);
  void set_pos(int x, int y, float rw = 0, float rh = 0);

  /** translate a SDL_Event into a menu_action */
  void event(SDL_Event& event);

  int get_width() const;
  int get_height() const;

  bool is_toggled(int id) const;
};

extern Surface* checkbox;
extern Surface* checkbox_checked;
extern Surface* back;
extern Surface* arrow_left;
extern Surface* arrow_right;

extern Menu* contrib_menu;
extern Menu* contrib_subset_menu;
extern Menu* main_menu;
extern Menu* game_menu;
extern Menu* worldmap_menu;
extern Menu* options_menu;
extern Menu* options_keys_menu;
extern Menu* options_joystick_menu;
extern Menu* options_joystick_axis_menu;
extern Menu* options_joystick_button_menu;
#ifdef USE_SDL2
extern Menu* options_gamepad_menu;
extern Menu* options_gamepad_analog_menu;
extern Menu* options_gamepad_device_menu;
extern Menu* options_touch_menu;
#endif
extern Menu* highscore_menu;
extern Menu* load_game_menu;
extern Menu* save_game_menu;

#endif /*SUPERTUX_MENU_H*/

/* Local Variables: */
/* mode: c++ */
/* End: */

// SPDX-FileCopyrightText: 2004 Ricardo Cruz <rick2@aeiou.pt>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_MOUSECURSOR_H
#define SUPERTUX_MOUSECURSOR_H

#include <string>
#include "timer.h"
#include "texture.h"

#define MC_FRAME_PERIOD 800  // in ms

#define MC_STATES_NB 3
enum {
  MC_NORMAL,
  MC_CLICK,
  MC_LINK
};

class MouseCursor
  {
    public:
    MouseCursor(std::string cursor_file, int frames);
    ~MouseCursor();
    int state();
    void set_state(int nstate);
    void set_mid(int x, int y);
    void draw();
    
    static MouseCursor* current() { return current_; };
    static void set_current(MouseCursor* pcursor) {  current_ = pcursor; };
    
    private:
    int mid_x, mid_y;
    static MouseCursor* current_;    
    int state_before_click;
    int cur_state;
    int cur_frame, tot_frames;
    Surface* cursor;
    Timer timer;
  };

#endif /*SUPERTUX_MOUSECURSOR_H*/

// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "platform_config.h"
#include <SDL_image.h>

#include "defines.h"
#include "globals.h"
#include "intro.h"
#include "text.h"

#include "screen.h"

void draw_intro()
{
if(debug_mode)
  fade("/images/background/arctis2.jpg", 30, false);
display_text_file("intro.txt", "/images/background/arctis2.jpg", SCROLL_SPEED_MESSAGE);
}


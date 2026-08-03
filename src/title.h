// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_TITLE_H
#define SUPERTUX_TITLE_H

/** Run the title screen (init + frame loop + shutdown). */
void title(void);

/** One iteration of the title/menu loop. Returns true while a menu is active. */
bool title_frame(void);

#endif /* SUPERTUX_TITLE_H */

// EOF //

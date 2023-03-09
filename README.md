# SuperTux Origins

SuperTux is a jump'n'run game with strong inspiration from the
Super Mario Bros. games for the various Nintendo platforms.

Run and jump through multiple worlds, fighting off enemies by jumping
on them, bumping them from below or tossing objects at them, grabbing
power-ups and other stuff on the way.

![Screenshot](https://www.supertux.org/images/0_6_0/0_6_0_3.png)

## Origins Fork

SuperTux Origins is a fork of the SuperTux project with the goal to
bring the project back down to more managable levels and get the game
finished. This means cutting out a lot of cruft:

* reversal of unnecessary graphic updates
* removal of addon and mod support
* removal of the build in level editor
* removal of OpenGL2 and SDL support
* removal of Discord integration
* removal of translation support
* removal of Github CI
* removal of unnecessary libraries (glbindings, ...)
* removal of unnecessary or low quality tileset, badguys and game objects
* removal of unnecessary powerups and actions (flowers, swimming)
* removal or cleanup of old levels (most of forest world might get cut/rebuild)
* focus on turning it back into a plain old jump'n run game, without convoluted puzzles or cutscenes
* focus on Linux/NixOS as the main target, abandonment of most ports

Some of the removed features might make it back eventually, but only
after the main game is in a more presentable state. No point in having
mod support when all it does is constantly break and bring development
to a crawl.

## Community

In case you need help, feel free to reach out using the following means:

* Matrix: https://matrix.to/#/#supertux-origins:matrix.org

## Run, Install and Develop

[Nix](https://nixos.org/download.html) is the only officially
supported platform, meaning it can work on most Linux distributions
via the Nix packgae manager, Mac might work too with Nix (untested)
and Windows is handled via cross-compile (see below). The [experimental
nix-command needs to be enabled](https://nixos.wiki/wiki/Nix_command).

To run the game:

    nix run github:supertux-origins/supertux

To install the game:

    nix profile install github:supertux-origins/supertux

To develop and build the game manually:

    git clone https://github.com/SuperTux-Origins/supertux.git
    cd supertux
    nix develop .
    mkdir build
    cd build
    cmake ..
    make

The Windows version can be cross-compiled from Linux with (32bit):

    nix build .#packages.i686-windows.supertux-origins-win32

or (64bit):

    nix build .#packages.x86_64-windows.supertux-origins-win32

## Documentation

Important documentation for SuperTux is contained in multiple files.
Please see them:

* `README.md` - This file
* `NEWS.md` - Changes since the previous versions of SuperTux.
* `LICENSE.txt` - The GNU General Public License, under whose terms SuperTux is
licensed. (Most of the data subdirectory is also licensed under
CC-by-SA)
* `data/credits.stxt` - Credits for people that contributed to the creation of
SuperTux. (You can view these in the game menu as well.)


## Playing the game

Both keyboards and joysticks/gamepads are supported. You can change
the controls via the Options menu. Basically, the only keys you will
need to use in-game are to do the following actions: jump, duck,
right, left, action and 'P' to pause/unpause the game. There isn't much
to tell about the first few, but the "action" key allows you to pick
up objects and use any powerup you got. For instance, with the fire
flower, you can shoot fireballs, or with the ice flower fire ice pellets.

Other useful keys include the Esc key, which is used to go to the menu
or to go up a level in the menu. The menu can be navigated using the
arrow keys or the mouse.

In the worldmap, the arrow keys are used to navigate and Enter to
enter the current level.

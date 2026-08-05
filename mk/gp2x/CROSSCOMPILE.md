# Cross-compiling for GP2X and Wiz (Open2x / OpenWiz)

This is the **primary reason** the tree still builds with **SDL 1.2**: both
handhelds ship community firmwares that expose SDL 1.2 (not SDL2). Desktop
defaults remain SDL2 via CMake; handheld targets force the SDL1 backend.

Status: **CMake + flake target compile**. Cross-builds with the historical
Open2x toolchain produce an ARM soft-float `.gpe`. Device smoke-test still
needed on hardware.

---

## Platforms

| Device | SoC / CPU | Screen | Typical firmware | Toolchain triple (common) |
|--------|-----------|--------|------------------|---------------------------|
| **GP2X** (F100/F200) | MagicEyes MMSP2, ARM920T | 320×240 | Open2x, GPH official | `arm-open2x-linux` |
| **GP2X Wiz** | ARM926EJ-S | 320×240 | OpenWiz / GPH | `arm-openwiz-linux-gnu` or `arm-gp2xwiz-linux-gnu` |

Both use the same in-tree defines for gameplay layout:

- `-DGP2X` — button IDs (`GP2X_BUTTON_*` in `globals.h`), joystick-as-buttons input, mikmod-era sound path
- `-DRES320X240` — 320×240 layout (HUD, scrolling, text positions)

Wiz binaries often reuse `-DGP2X` because the SDL joystick button numbering is
close enough; confirm on hardware before shipping a Wiz-only build.

**SDL2 is not viable** on stock Open2x/OpenWiz userspace. Keep
`-DENABLE_SDL2=OFF` for these targets.

---

## Toolchains

### Open2x (GP2X) — recommended historical path

Canonical apps toolchain (GCC 4.1.1 + glibc 2.3.6):

| Package | Typical install root |
|---------|----------------------|
| Apps toolchain | `/opt/open2x/gcc-4.1.1-glibc-2.3.6` |
| Libpack (SDL, image, …) | same prefix (`include/`, `lib/`) |

Binaries are named `arm-open2x-linux-{gcc,g++,ar,strip,…}`.

Historical download names (mirrors vary; SourceForge Open2x era):

- `arm-open2x-linux-apps-gcc-4.1.1-glibc-2.3.6_i686_linux.tar.bz2`
- `open2x-libpack-20071903-gcc-4.1.1-glibc-2.3.6.tar.bz2`

Build-from-source (legacy SVN; may be dead):

```text
svn co https://open2x.svn.sourceforge.net/svnroot/open2x/trunk/toolchain-new
./open2x-gp2x-apps.sh   # installs under /opt/open2x by default
```

Wiki reference (archive-era): `http://wiki.gp2x.org/articles/i/n/s/Installing_the_Open2x_toolchain.html`

### OpenWiz (Wiz)

Common prefixes:

- `/opt/openwiz/toolchain` with `arm-openwiz-linux-gnu-*`
- Or `arm-gp2xwiz-linux-gnu-*` with `WIZ_HOME` / `OPENWIZ` pointing at the sysroot

SDL 1.2 is cross-built into the toolchain prefix (`--host=arm-openwiz-linux-gnu`
or equivalent). Link with `-I$PREFIX/include/SDL -L$PREFIX/lib -lSDL …`.

### Soft-float and static link

Older Open2x userlands expect **soft-float** (`-msoft-float`) and often
**static** linking for a single `.gpe` that does not depend on random SD card
`.so` versions. Prefer static when packaging for GMenu2X.

---

## Required libraries (SDL 1.2 stack)

| Library | Role | Notes |
|---------|------|--------|
| **SDL 1.2** | Video, events, joystick | From Open2x libpack or self-built |
| **SDL_image** | PNG/JPEG surfaces | Needs libpng, libjpeg, zlib in the same prefix |
| **zlib** | lispreader / compression | Usually in libpack |
| **SDL_mixer** | Optional desktop-style audio | Prefer off for first GP2X bring-up |
| **libmikmod** | Historical GP2X music path in `sound.cpp` | Only if `-DENABLE_SOUND=ON` under `-DGP2X` |
| **SDL_gfx** | Required by **old** Autotools GP2X configure | **Not referenced** by current `src/`; skip unless a call site returns |

Default CMake handheld profile: **no OpenGL**, **no sound** (silence), SDL1 only.
That matches the parked `gp2x_configure` flags:
`--enable-320x240 --enable-gp2x --disable-opengl --enable-silence`.

---

## In-tree engine expectations (`-DGP2X`)

Already present (do not invent parallel button schemes):

- `globals.h` — `GP2X_BUTTON_*` constants and `JoystickKeymap` GP2X fields
- `setup.cpp` — GP2X joystick open / volume buttons
- `gameloop.cpp` / `worldmap.cpp` / `menu.cpp` — `SDL_JOYBUTTON*` mapped via `joystick_keymap`
- `sound.cpp` — mikmod + `updateSound()` pump when not `NOSOUND`
- `RES320X240` — tighter HUD and camera constants

Level editor and heavy desktop-only paths should stay disabled or unused on device
(binary size + input).

---

## CMake build (supported path going forward)

Toolchain files live next to this document:

- `toolchain-open2x.cmake` — Open2x apps toolchain
- `toolchain-openwiz.cmake` — OpenWiz

Example (GP2X, silence, 320×240):

```bash
# Adjust OPEN2X_ROOT to your install
export OPEN2X_ROOT=/opt/open2x/gcc-4.1.1-glibc-2.3.6
export PATH="$OPEN2X_ROOT/bin:$PATH"

cmake -S . -B build-gp2x \
  -DCMAKE_TOOLCHAIN_FILE=mk/gp2x/toolchain-open2x.cmake \
  -DENABLE_GP2X=ON \
  -DENABLE_RES320X240=ON \
  -DENABLE_SDL2=OFF \
  -DENABLE_OPENGL=OFF \
  -DENABLE_SOUND=OFF \
  -DOPEN2X_ROOT="$OPEN2X_ROOT" \
  -DDATA_PREFIX=.

cmake --build build-gp2x -j"$(nproc)"
arm-open2x-linux-strip build-gp2x/supertux-milestone1
# Ship as supertux-milestone1.gpe + data/ on the SD card
```

Wiz:

```bash
export OPENWIZ_ROOT=/opt/openwiz/toolchain
cmake -S . -B build-wiz \
  -DCMAKE_TOOLCHAIN_FILE=mk/gp2x/toolchain-openwiz.cmake \
  -DENABLE_GP2X=ON \
  -DENABLE_RES320X240=ON \
  -DENABLE_SDL2=OFF \
  -DENABLE_OPENGL=OFF \
  -DENABLE_SOUND=OFF \
  -DOPENWIZ_ROOT="$OPENWIZ_ROOT"
```

`OPEN2X_ROOT` / `OPENWIZ_ROOT` tell CMake where to find headers and static/shared
SDL libs without relying on host `pkg-config`.

### Packaging for GMenu2X

1. Binary name often ends in `.gpe` (GP2X) or `.gpu` / `.gpe` (Wiz conventions vary).
2. Place `data/` next to the binary or set `DATA_PREFIX` / runtime discovery to the
   install folder on the SD card.
3. Optional: wrapper script that `cd`s to the game directory then `exec`s the binary
   (so relative `data/` works).
4. GMenu2X link: working directory = game folder; icon optional.

---

## Autotools (parked, reference only)

`mk/gp2x/configure.ac` + `gp2x_configure` / `gp2x_make` remain as historical
wrappers. Source lists are **incomplete** vs current `src/` (missing
`app_loop`, `platform_sdl1`, touch controls, etc.). Prefer CMake.

Legacy configure shape:

```bash
./configure --host=arm-open2x-linux \
  --enable-gp2x --enable-320x240 \
  --disable-opengl --enable-silence
```

---

## Risks and next implementation steps

| Item | Risk | Next step |
|------|------|-----------|
| Toolchain availability | Open2x/OpenWiz downloads are aging; 32-bit host tools | Document mirror; optional Nix fixed-output derivation later |
| Soft-float ABI | Wrong float ABI → silent crash on device | Toolchain file sets `-msoft-float` for Open2x |
| Sound | mikmod vs SDL_mixer paths under `GP2X` | Keep `ENABLE_SOUND=OFF` until mikmod is wired in CMake |
| SDL_gfx | Old configure required it; code may not | Do not require unless a symbol is needed |
| Host `pkg-config` | Points at desktop SDL | Always use `OPEN2X_ROOT` / toolchain file |
| Data size | Full `data/` is large for SD | Optional slim data set later |
| Device test | No emulator in CI | Manual GP2X/Wiz playtest checklist |

**Immediate CMake work (started in-tree):**

1. `ENABLE_GP2X` → `-DGP2X`
2. `ENABLE_RES320X240` → `-DRES320X240`
3. Force SDL1 + no GL when `ENABLE_GP2X=ON`
4. `OPEN2X_ROOT` / `OPENWIZ_ROOT` import of SDL 1.2 + SDL_image
5. Toolchain files under `mk/gp2x/`

**Follow-ups (not done yet):**

- Static link helper (`-static` + correct lib order: image, SDL, pthread, m, z, jpeg, png)
- Optional mikmod for `ENABLE_SOUND` under GP2X
- `nix` package that only builds when a toolchain path is provided
- Device smoke checklist (boot → title → level → volume keys)

---

## Nix flake (recommended)

```bash
nix build .#supertux-milestone1-gp2x   # classic GP2X / Open2x
nix build .#supertux-milestone1-wiz    # GP2X Wiz / GPH SDK
# → result/bin/supertux-milestone1.gpe
```

Supporting packages:

| Attribute | Purpose |
|-----------|---------|
| `open2x-sysroot` | Open2x gcc-4.1.1 + libpack (SDL 1.2) |
| `openwiz-sysroot` | GPH_SDK 10.02 (`arm-linux` + DGE SDL 1.2) |
| `supertux-milestone1-gp2x` | GP2X Open2x `.gpe` |
| `supertux-milestone1-wiz` | Wiz GPH `.gpe` |

The flake downloads the Open2x toolchain and libpack from the nanard.free.fr
mirrors (same hashes as the grafx2 install script). Host tools are **i686**;
on x86_64 without IA32 emulation, wrappers use `qemu-i386`.

Manual CMake path remains supported when `OPEN2X_ROOT` is already installed
under `/opt/open2x/…`.


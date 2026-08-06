# Cross-compiling SuperTux Milestone 1 for R36S (ArkOS)

**Target hardware:** R36S handheld — Rockchip **RK3326** (4× Cortex-A35), **640×480** panel, Mali-G31 GPU.  
**Typical firmware:** [ArkOS](https://github.com/christianhaitian/arkos) (and community R36S images / dArkOS). ArkOS is a **modified Ubuntu 19.10 (eoan)** userspace for Rockchip handhelds, with **both aarch64 and armhf** library trees.

Game logical resolution is already **640×480** (`ST_SCREEN_W` / `ST_SCREEN_H`). Do **not** enable `RES320X240` / `ENABLE_GP2X` for this device.

| Setting | Value |
|---------|--------|
| Backend | **SDL2** (`ENABLE_SDL2=ON`) |
| Renderer | **GLES2** (`ENABLE_GLES2=ON`) — Mali-G31 / libmali or Panfrost |
| Sound | SDL2_mixer when available (`ENABLE_SOUND=ON`) |
| ABI (preferred) | **aarch64-linux-gnu** |
| ABI (PortMaster-style) | **arm-linux-gnueabihf** (optional second package) |

---

## Why a sysroot matters (glibc)

Stock ArkOS tracks **Ubuntu 19.10**-era libraries. That means roughly:

| Component | Typical on device |
|-----------|-------------------|
| glibc | **2.30** (eoan); some images stay near Debian Buster’s **2.28** |
| libstdc++ | GCC 8/9 era |
| SDL2 | Custom ArkOS builds (often 2.0.x with KMS/DRM patches), not stock Ubuntu only |

A binary linked against **modern nixpkgs / Ubuntu 22.04+ glibc** will fail on the device with:

```text
/lib/aarch64-linux-gnu/libc.so.6: version `GLIBC_2.3x' not found
```

So for a binary that runs on **stock ArkOS**, you must link against headers and shared libraries from a **matching or older** userspace (device copy, eoan/buster chroot, or equivalent sysroot).

Building with current `pkgsCross.aarch64-multiplatform` is still useful to prove the tree cross-compiles and to run under `qemu-aarch64`; it is **not** a drop-in ArkOS package without a compatible sysroot or shipping a full private `libs/` tree built for the same glibc.

---

## Recommended paths (pick one)

### A. Sysroot from the device (most accurate)

On a host with `aarch64-linux-gnu-gcc` (Debian/Ubuntu: `gcc-aarch64-linux-gnu`):

```bash
# On the R36S (SSH user often "ark" / password "ark"):
sudo apt-get update
sudo apt-get install --reinstall -y \
  libc6-dev linux-libc-dev libstdc++-9-dev \
  libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev \
  libgles2-mesa-dev libegl1-mesa-dev libgbm-dev \
  zlib1g-dev libpng-dev pkg-config

# From the build machine:
./mk/r36s/scripts/make-sysroot-from-device.sh ark@<device-ip> $HOME/arkos-sysroot-aarch64
```

Then:

```bash
export ARKOS_SYSROOT=$HOME/arkos-sysroot-aarch64
cmake -S . -B build-r36s \
  -DCMAKE_TOOLCHAIN_FILE=mk/r36s/toolchain-arkos-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_SDL2=ON -DENABLE_GLES2=ON -DENABLE_SOUND=ON \
  -DDATA_PREFIX=./data
cmake --build build-r36s -j$(nproc)
```

Copy `build-r36s/supertux-milestone1` and the `data/` tree to the device (e.g. under `/roms/ports/supertux-milestone1/`).

### B. Debootstrap an old arm64 rootfs (no device required)

Debian **Buster** (glibc 2.28) is a safe ABI floor and still has SDL2 in archives:

```bash
sudo ./mk/r36s/scripts/make-sysroot-debootstrap.sh /opt/arkos-sysroot-buster arm64
export ARKOS_SYSROOT=/opt/arkos-sysroot-buster
# same cmake line as above
```

Ubuntu **eoan** packages live on `old-releases.ubuntu.com` if you need a closer match; Buster is usually enough if you avoid brand-new symbols.

### C. Build *on* the device

ArkOS ships with incomplete `-dev` packages until reinstalled (see [r36s-programming](https://github.com/dov/r36s-programming)):

```bash
sudo apt-get install --reinstall -y \
  g++ cmake ninja-build pkg-config \
  libc6-dev libstdc++-9-dev libsdl2-dev libsdl2-image-dev \
  libsdl2-mixer-dev libgles2-mesa-dev zlib1g-dev
cmake -S . -B build -DENABLE_SDL2=ON -DENABLE_GLES2=ON
cmake --build build -j2
```

Slow (1 GB RAM) but guaranteed ABI match.

### D. Nix flake + published ArkOS sysroot (preferred for device)

```bash
nix build .#arkos-sysroot              # unpacks the published tarball
nix build .#supertux-milestone1-r36s   # links against that sysroot
```

Sysroot URL (flake `fetchurl`):

```text
https://github.com/grumnix/arkos-sysroot.tar.gz
```

If the hash in `nix/r36s.nix` is still a placeholder, the first build fails with
the correct `sha256-…` — paste it into `arkosSysrootSrc.hash` and rebuild.

The resulting binary uses the **sysroot’s glibc/SDL2/GLES**, not nixpkgs Mesa,
so it should not look for `/run/opengl-driver`.

### E. Nix flake pkgsCross only (newer glibc — qemu/CI)



```bash
nix build .#supertux-milestone1-r36s
# → result/bin/supertux-milestone1 (aarch64, SDL2+GLES2)
# Test: qemu-aarch64 -L <sysroot> result/bin/supertux-milestone1
```

This uses nixpkgs `pkgsCross.aarch64-multiplatform` and **will not** run on stock ArkOS without a private library set or a newer userspace (Arch-R, Rocknix, etc.).

---

## Obtaining compatible libraries and headers

| Source | Pros | Cons |
|--------|------|------|
| **Device rsync** (`make-sysroot-from-device.sh`) | Exact SDL2/Mali/glibc as on hardware | Needs SSH + disk space |
| **Debian Buster arm64 debootstrap** | Reproducible, no device | SDL may differ from ArkOS KMS build |
| **Ubuntu 19.10 (eoan) ports** | Closest to ArkOS base | EOL; use old-releases mirrors |
| **ArkOS build chroots** (wiki) | Matches maintainer workflow | Heavy (qemu-user + full chroot) |
| **nixpkgs pkgsCross** | Pure flake, good for CI | glibc too new for stock ArkOS |

**Runtime deps on device (shared):** `libSDL2-2.0.so.0`, `libSDL2_image-2.0.so.0`, `libSDL2_mixer-2.0.so.0` (if sound), `libGLESv2`, `libEGL`, `libgbm`, `libdrm`, `libz`, `libpng` (often via SDL_image).

ArkOS maintains **custom SDL2** builds (DRM/KMS). Linking against stock Debian SDL2 headers is usually OK; at **run** time prefer the device’s `libSDL2.so`.

**GPU:** Mali-G31 via proprietary **libmali** (GBM) or open **Panfrost**. GLES2 context creation goes through SDL2; ensure the image’s EGL/GLES stack is intact. No need for desktop GLU on this path.

---

## CMake toolchain files

| File | Triple |
|------|--------|
| `mk/r36s/toolchain-arkos-aarch64.cmake` | `aarch64-linux-gnu` |
| `mk/r36s/toolchain-arkos-armhf.cmake` | `arm-linux-gnueabihf` |

Both expect:

- `ARKOS_SYSROOT` — absolute path to rootfs (or pass `-DARKOS_SYSROOT=…`)
- Cross compilers on `PATH` (`aarch64-linux-gnu-gcc` / `arm-linux-gnueabihf-gcc`)

---

## Deploy sketch (PortMaster / ES)

```text
/roms/ports/supertux-milestone1/
  supertux-milestone1          # binary
  data/                        # game data
  supertux-milestone1.sh       # launcher (optional)
```

Example launcher:

```bash
#!/bin/bash
cd "$(dirname "$0")"
export SDL_GAMECONTROLLERCONFIG="${SDL_GAMECONTROLLERCONFIG}"
# Optional if you ship private libs built against device glibc:
# export LD_LIBRARY_PATH="$PWD/libs:$LD_LIBRARY_PATH"
./supertux-milestone1 --fullscreen
```

Map the built-in gamepad via SDL2 gamecontroller (ArkOS already installs a controller database for many pads).

---

## References

- [ArkOS Building wiki](https://github.com/christianhaitian/ArkOS/wiki/Building) — Ubuntu 19.10 base, arm64/armhf chroots, Mali + SDL2 notes  
- [dov/r36s-programming](https://github.com/dov/r36s-programming) — on-device C/SDL2 setup, 640×480 hardware notes  
- [PortMaster packaging notes](https://github.com/christianhaitian/arkos/blob/main/ports/docs/packaging.md) — `libs/`, `SDL_GAMECONTROLLERCONFIG`, multi-device RK3326  
- Game defaults: logical **640×480** backbuffer in `platform_sdl2.cpp` / `defines.h` (matches R36S panel)

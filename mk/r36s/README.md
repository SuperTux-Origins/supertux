# R36S / ArkOS packaging (SuperTux Origins)

Scaffolding adapted from Pingus / Windstille.

## Hybrid toolchain

See `CROSSCOMPILE.md` and `nix/r36s.nix`. Summary:

| Piece | Source |
|-------|--------|
| Compiler | nixpkgs aarch64 cross GCC |
| C++ headers | same GCC |
| libc headers / libstdc++ | ArkOS sysroot |
| libgcc | static from toolchain or matching shared |
| Dynamic linker | `/lib/ld-linux-aarch64.so.1` |

## SuperTux-specific notes

- Target GLES2 (`ENABLE_OPENGLES2=ON`); do not link desktop GLEW/libGL.
- Force 640×480 / non-resizable handheld defaults in config or launcher.
- Controller profile: SDL_GameControllerButton layout (DPAD 11–14).
- Skip fragile `set_icon` / texture paths that throw into broken unwind
  (see PORTING.md / Windstille `WINDSTILLE_R36S` pattern — introduce
  `SUPERTUX_R36S` if needed).
- Sysroot URL in `nix/r36s.nix` is still a localhost placeholder until a
  permanent tarball is published.

## Outputs (planned)

- `arkos-sysroot` — unpacked sysroot derivation
- `supertux-r36s` — game binary linked against sysroot
- `supertux-r36s-portmaster` — PortMaster layout under `/roms/ports`

## Flake outputs

| Attribute | Description |
|-----------|-------------|
| `arkos-sysroot` | Unpacked sysroot derivation |
| `supertux-r36s` | Cross-built game binary |
| `supertux-r36s-portmaster` | PortMaster tree |
| `supertux-r36s-portmaster-zip` | Zip for autoinstall |

Override the sysroot tarball by setting `sysrootSrc` when importing
`nix/r36s.nix` from `flake.nix` (see PORTING.md).

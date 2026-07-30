#!/usr/bin/env bash
# Collects sdl-libs' build outputs into $out. Expects:
#   TARGET_ABIS - space-separated list, e.g. "armeabi-v7a arm64-v8a"
#   $out        - set by Nix's installPhase automatically
set -euo pipefail

mkdir -p "$out/lib" "$out/dex"
for abi in $TARGET_ABIS; do
  mkdir -p "$out/lib/$abi"
  cp sdl-jni/libs/"$abi"/*.so "$out/lib/$abi/"
done
cp -r sdl-jni/SDL/include "$out/include"
cp classes/classes.dex "$out/dex/classes.dex"

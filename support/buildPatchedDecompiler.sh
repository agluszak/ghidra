#!/usr/bin/env bash
# Build this fork's native fixes against the installed release's source and ABI.
set -euo pipefail
install_dir="$(realpath "${1:?Ghidra installation directory}")"
revision="${2:?Pinned fork commit}"
patch_file="$(mktemp)"
trap 'rm -f "$patch_file"' EXIT
curl --fail --location --retry 3 --output "$patch_file" \
  "https://github.com/agluszak/ghidra/commit/$revision.patch"
(
  cd "$install_dir"
  git apply --include=Ghidra/Features/Decompiler/src/decompile/cpp/flow.cc "$patch_file"
)
cpp="$install_dir/Ghidra/Features/Decompiler/src/decompile/cpp"
make -C "$cpp" -j6 ghidra_opt
install -m755 "$cpp/ghidra_opt" \
  "$install_dir/Ghidra/Features/Decompiler/os/linux_x86_64/decompile"

#!/usr/bin/env bash
# Build this fork's native fixes against the installed release's source and ABI.
set -euo pipefail
install_dir="$(realpath "${1:?Ghidra installation directory}")"
patch_file="$(mktemp)"
trap 'rm -f "$patch_file"' EXIT
# Apply the release-compatible continuation fix, then operation-identity cloning.
for revision in \
  78cca264d0daa7d3827db10cfa1150fc7cfced67 \
  0b630c60b8f886e27ae57e20fa43175654b52f5f; do
  curl --fail --location --retry 3 --output "$patch_file" \
    "https://github.com/agluszak/ghidra/commit/$revision.patch"
  (
    cd "$install_dir"
    git apply --include='Ghidra/Features/Decompiler/src/decompile/cpp/*' "$patch_file"
  )
done
cpp="$install_dir/Ghidra/Features/Decompiler/src/decompile/cpp"
make -C "$cpp" -j6 ghidra_opt
module="$install_dir/Ghidra/Features/Decompiler"
# Ghidra prefers a module's build/os executable over its released os copy.
native_dir="$module/os/linux_x86_64"
if [[ -f "$module/build/os/linux_x86_64/decompile" ]]; then
  native_dir="$module/build/os/linux_x86_64"
fi
install -m755 "$cpp/ghidra_opt" "$native_dir/decompile"

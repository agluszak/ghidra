#!/usr/bin/env bash
# Install one checksum-pinned Linux distribution published by this fork.
set -euo pipefail
revision="${1:?Fork commit}"
expected_sha256="${2:?Archive SHA256}"
install_dir="${3:?Ghidra installation directory}"
test ! -e "$install_dir"
parent="$(dirname "$install_dir")"
mkdir -p "$parent"
scratch="$(mktemp -d "$parent/.ghidra-install.XXXXXX")"
trap 'rm -rf "$scratch"' EXIT
curl --fail --location --retry 3 --output "$scratch/ghidra-linux.zip" \
  "https://github.com/agluszak/ghidra/releases/download/wizardry-$revision/ghidra-linux.zip"
printf '%s  %s\n' "$expected_sha256" "$scratch/ghidra-linux.zip" | sha256sum --check -
unzip -q "$scratch/ghidra-linux.zip" -d "$scratch/unpacked"
roots=("$scratch"/unpacked/ghidra_*)
test "${#roots[@]}" -eq 1
root="${roots[0]}"
grep -Fx "application.fork.revision=$revision" "$root/Ghidra/application.properties"
test -x "$root/support/analyzeHeadless"
test -x "$root/Ghidra/Features/Decompiler/os/linux_x86_64/decompile"
mv "$root" "$install_dir"

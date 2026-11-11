#!/usr/bin/env bash
set -euo pipefail

TARGET_DIR="$(pwd)/kernel"
REPO_URL="https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"

mkdir -p "$TARGET_DIR"
git init -b master "$TARGET_DIR"
cd "$TARGET_DIR"

# check if orgin exists if so just set.
if git remote get-url origin >/dev/null 2>&1; then
  echo "Setting Repo Orgin to: '$REPO_URL'"
  git remote set-url origin "$REPO_URL"
else
  echo "Adding Repo Origin of: '$REPO_URL'"
  git remote add origin "$REPO_URL"
fi

git sparse-checkout set --cone \
  include \
  scripts \
  tools \
  arch/x86 \
  arch/arm64 \
  arch/riscv

git fetch --depth=1 origin master
git checkout -B master FETCH_HEAD

build_target() {
  local label="$1"
  local arch="$2"
  local bits="$3"

  local build="$TARGET_DIR/build/$label"
  local install="$TARGET_DIR/install/$label"

  mkdir -p $build
  mkdir -p $install

  echo Label: $label Arch: $arch

  #https://docs.kernel.org/kbuild/headers_install.html
  make O="$build" ARCH="$arch" headers_install INSTALL_HDR_PATH="$install"
}

build_target "x86_64"  "x86"  
build_target "arm64"   "arm64"
build_target "riscv"   "riscv"

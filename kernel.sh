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

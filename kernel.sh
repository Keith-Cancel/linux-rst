#!/usr/bin/env bash
set -euo pipefail

TARGET_DIR="$(pwd)/kernel"
REPO_URL="https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"

mkdir -p "$TARGET_DIR"
echo "Getting Linux headers from: '$REPO_URL'"

git init "$TARGET_DIR"
cd "$TARGET_DIR"
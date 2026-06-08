#!/usr/bin/env bash
#
# build.sh — task runner for the EasyWiFi library.
#
# Usage:
#   ./build.sh <action> [action ...]
#
# Actions:
#   clean     Remove build artifacts (.pio caches and dist/).
#   build     Compile the example against the library, then pack the
#             distributable tarball into dist/.
#   publish   Publish the packed tarball to the PlatformIO Registry.
#             Requires `pio account login` first; pio prompts for confirmation.
#
# Actions run in the order given and the script STOPS on the first failure:
#   ./build.sh clean build publish
#
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$ROOT/examples/Lantern"
DIST_DIR="$ROOT/dist"

# --- output helpers --------------------------------------------------------
if [ -t 1 ]; then
  BOLD="$(printf '\033[1m')"; RED="$(printf '\033[31m')"
  GREEN="$(printf '\033[32m')"; BLUE="$(printf '\033[34m')"; RESET="$(printf '\033[0m')"
else
  BOLD=""; RED=""; GREEN=""; BLUE=""; RESET=""
fi
info()  { echo "${BLUE}${BOLD}==>${RESET} $*"; }
ok()    { echo "${GREEN}✓${RESET} $*"; }
fail()  { echo "${RED}✗ $*${RESET}" >&2; }

version() {
  python3 -c "import json;print(json.load(open('$ROOT/library.json'))['version'])"
}

require_pio() {
  command -v pio >/dev/null 2>&1 || { fail "PlatformIO CLI (pio) not found on PATH."; return 1; }
}

# --- actions ---------------------------------------------------------------
do_clean() {
  info "clean: removing build artifacts"
  rm -rf "$DIST_DIR" || return 1
  rm -rf "$ROOT/.pio" || return 1
  rm -rf "$EXAMPLE_DIR/.pio" || return 1
  ok "clean complete"
}

do_build() {
  require_pio || return 1
  local ver tarball
  ver="$(version)" || { fail "could not read version from library.json"; return 1; }

  info "build: compiling example (verifies the library builds)"
  pio run -d "$EXAMPLE_DIR" || { fail "example build failed"; return 1; }

  info "build: packing library tarball"
  mkdir -p "$DIST_DIR" || return 1
  tarball="$DIST_DIR/EasyWiFi-$ver.tar.gz"
  pio pkg pack -o "$tarball" || { fail "pio pkg pack failed"; return 1; }
  ok "packed $tarball"
}

do_publish() {
  require_pio || return 1
  local ver tarball
  ver="$(version)" || { fail "could not read version from library.json"; return 1; }
  tarball="$DIST_DIR/EasyWiFi-$ver.tar.gz"

  # Ensure there is something to publish.
  if [ ! -f "$tarball" ]; then
    info "publish: no tarball found, packing first"
    mkdir -p "$DIST_DIR" || return 1
    pio pkg pack -o "$tarball" || { fail "pio pkg pack failed"; return 1; }
  fi

  # Require an authenticated PlatformIO account.
  if ! pio account show >/dev/null 2>&1; then
    fail "not logged in to PlatformIO. Run: pio account login"
    return 1
  fi

  info "publish: publishing $tarball (v$ver) to the PlatformIO Registry"
  # No --no-interactive: pio shows its own confirmation prompt before publishing.
  pio pkg publish "$tarball" || { fail "pio pkg publish failed"; return 1; }
  ok "published EasyWiFi v$ver"
}

run_action() {
  case "$1" in
    clean)   do_clean ;;
    build)   do_build ;;
    publish) do_publish ;;
    *) fail "unknown action: '$1' (expected: clean | build | publish)"; return 2 ;;
  esac
}

usage() {
  cat <<'EOF'
build.sh — task runner for the EasyWiFi library.

Usage:
  ./build.sh <action> [action ...]

Actions:
  clean     Remove build artifacts (.pio caches and dist/).
  build     Compile the example against the library, then pack the
            distributable tarball into dist/.
  publish   Publish the packed tarball to the PlatformIO Registry
            (run `pio account login` first; pio prompts for confirmation).

Actions run in the order given and STOP on the first failure:
  ./build.sh clean build publish
EOF
}

main() {
  if [ "$#" -eq 0 ]; then
    usage
    exit 1
  fi
  for action in "$@"; do
    if ! run_action "$action"; then
      fail "stopped: action '$action' failed"
      exit 1
    fi
  done
  echo
  ok "all actions completed: $*"
}

main "$@"

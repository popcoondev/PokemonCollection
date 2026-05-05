#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PRESET="${1:-}"
LOCAL_DEPS_DIR="$SCRIPT_DIR/local-deps"
LOCAL_ARDUINOJSON_DIR="$LOCAL_DEPS_DIR/ArduinoJson"

if [[ -z "$PRESET" ]]; then
  if [[ "$(uname -s)" == "Darwin" && "$(uname -m)" == "arm64" ]]; then
    PRESET="macos-arm64"
  else
    PRESET="default"
  fi
fi

die() {
  echo "error: $*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "'$1' is required"
}

need_cmd cmake

if [[ "$PRESET" == "macos-arm64" ]]; then
  need_cmd brew
  for pkg in sdl2 sdl2_image sdl2_ttf; do
    if [[ ! -d "/opt/homebrew/opt/$pkg" ]]; then
      die "Homebrew package '$pkg' is missing. Run: brew install cmake sdl2 sdl2_image sdl2_ttf"
    fi
  done
fi

resolve_arduinojson_dir() {
  local candidate="$1"
  if [[ -z "$candidate" ]]; then
    return 1
  fi
  if [[ -f "$candidate/src/ArduinoJson.h" ]]; then
    printf '%s\n' "$candidate"
    return 0
  fi
  if [[ -f "$candidate/ArduinoJson.h" ]]; then
    printf '%s\n' "$(cd "$candidate/.." && pwd)"
    return 0
  fi
  return 1
}

copy_arduinojson_into_local_deps() {
  local source_dir="$1"
  mkdir -p "$LOCAL_DEPS_DIR"
  rm -rf "$LOCAL_ARDUINOJSON_DIR"
  cp -R "$source_dir" "$LOCAL_ARDUINOJSON_DIR"
  printf '%s\n' "$LOCAL_ARDUINOJSON_DIR"
}

find_arduinojson_root() {
  local resolved=""
  if [[ -n "${ARDUINOJSON_ROOT:-}" ]]; then
    if resolved="$(resolve_arduinojson_dir "${ARDUINOJSON_ROOT}")"; then
      printf '%s\n' "$resolved"
      return 0
    fi
    die "ARDUINOJSON_ROOT is set, but ArduinoJson.h was not found under '$ARDUINOJSON_ROOT'"
  fi

  if resolved="$(resolve_arduinojson_dir "$LOCAL_ARDUINOJSON_DIR")"; then
    printf '%s\n' "$resolved"
    return 0
  fi

  local candidate
  for candidate in \
    "$SCRIPT_DIR/../.pio/libdeps"/*/ArduinoJson \
    "$HOME/.platformio/lib/ArduinoJson" \
    "$HOME/Documents/Arduino/libraries/ArduinoJson" \
    "/tmp/ArduinoJson"
  do
    if resolved="$(resolve_arduinojson_dir "$candidate")"; then
      printf '%s\n' "$resolved"
      return 0
    fi
  done

  return 1
}

ARDUINOJSON_ROOT_VALUE=""
if ARDUINOJSON_ROOT_VALUE="$(find_arduinojson_root)"; then
  if [[ "$ARDUINOJSON_ROOT_VALUE" != "$LOCAL_ARDUINOJSON_DIR" ]]; then
    echo "Caching ArduinoJson into: $LOCAL_ARDUINOJSON_DIR"
    ARDUINOJSON_ROOT_VALUE="$(copy_arduinojson_into_local_deps "$ARDUINOJSON_ROOT_VALUE")"
  fi
  echo "Using ArduinoJson from: $ARDUINOJSON_ROOT_VALUE"
  cmake --preset "$PRESET" -DARDUINOJSON_ROOT="$ARDUINOJSON_ROOT_VALUE"
  exit 0
fi

cat >&2 <<'EOF'
error: ArduinoJson was not found locally.

Do one of the following, then rerun this script:
  1. Export ARDUINOJSON_ROOT=/path/to/ArduinoJson
  2. Run `pio run -e m5stack-cores3` once to populate .pio/libdeps
  3. Re-run CMake directly with network access so FetchContent can download ArduinoJson
EOF
exit 1

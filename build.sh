#!/bin/bash

# If any simple command fails, everything stops
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

CLEAN_FLAG=false
RUN_FLAG=false
DEBUG_FLAG=false
ASAN_FLAG=false
TSAN_FLAG=false

# Args: c=clean, r=run app, d=debug, a=ASan+UBSan, t=TSan
while getopts "crdat" opt; do
  case $opt in
    c) echo "=== Clean build ==="
      CLEAN_FLAG=true
      ;;
    r) echo "=== Will Run the Project after the build ==="
      RUN_FLAG=true
      ;;
    a) echo "=== ASan + UBSan ==="
      ASAN_FLAG=true
      ;;
    d) echo "=== Debug build ==="
      DEBUG_FLAG=true
      ;;
    t) echo "=== Thread sanitizer build ==="
      TSAN_FLAG=true
      ;;
    *) echo "=== Unknown option ===" ;;
  esac
done

build() {
  if [[ $CLEAN_FLAG == "true" ]]; then
    rm -rf "${SCRIPT_DIR}/build/"
  fi

  mkdir -p "${SCRIPT_DIR}/build"
  mkdir -p "${SCRIPT_DIR}/bin"

  echo "=== Starting CMake build ==="
  cd "${SCRIPT_DIR}/build"

  CMAKE_ARGS=(
    # Main app + core + strategy: do not pull in gtest / test targets
    -DTRADINGBOT_BUILD_TESTS=OFF
  )

  # --- Build type
  if [[ $DEBUG_FLAG == "true" || $TSAN_FLAG == "true" ]]; then
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Debug)
  else
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Release)
  fi

  # --- Sanitizers (apply to all targets via global flags; exe link completes ASan/TSan)
  if [[ $ASAN_FLAG == "true" ]]; then
    echo "=== Using ASan + UBSan ==="
    CMAKE_ARGS+=(
      "-DCMAKE_CXX_FLAGS=-O0 -g -fsanitize=address,undefined"
      "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"
    )
  fi

  if [[ $TSAN_FLAG == "true" ]]; then
    echo "=== Using ThreadSanitizer ==="
    CMAKE_ARGS+=(
      "-DCMAKE_CXX_FLAGS=-O1 -g -fsanitize=thread"
      "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread"
    )
  fi

  cmake .. "${CMAKE_ARGS[@]}"

  if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
  else
    JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
  fi
  cmake --build . --parallel "${JOBS}"

  echo "=== Finished CMake build ==="
}

run() {
  echo "=== Bot is started ==="
  cd "${SCRIPT_DIR}"

  BOT_BIN="${SCRIPT_DIR}/bin/TradingBot"
  if [[ ! -x "$BOT_BIN" ]]; then
    echo "Error: executable not found or not executable: $BOT_BIN" >&2
    exit 1
  fi

  # first argument is absolute path to .env,
  # second argument is absolute path to binance PK
  "$BOT_BIN" "${SCRIPT_DIR}/.env" "${SCRIPT_DIR}/binance_private.pem"
  echo "=== Bot is stopped ==="
}

build

if [[ $RUN_FLAG == "true" ]]; then
  run
fi

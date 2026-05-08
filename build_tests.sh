#!/bin/bash

# If any simple command fails, everything stops
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

TARGET_NAME="TradingBotTests"

RUN_FLAG=false
VERBOSE_FLAG=false
CLEAN_FLAG=false
COVERAGE_FLAG=false
FILTERED_TEST=""

# Args: c=clean, g=coverage, f=gtest filter, r=run tests, V=verbose ctest
while getopts "cgf:rV" opt; do
  case $opt in
    c)
      echo "=== Clean test build ==="
      CLEAN_FLAG=true
      ;;
    g)
      echo "=== Will enable coverage report ==="
      COVERAGE_FLAG=true
      ;;
    f)
      echo "=== Filtering tests: $OPTARG ==="
      FILTERED_TEST=$OPTARG
      ;;
    r)
      echo "=== Will run tests after the build ==="
      RUN_FLAG=true
      ;;
    V)
      echo "=== Verbose ctest ==="
      VERBOSE_FLAG=true
      ;;
    *)
      echo "=== Unknown option ==="
      ;;
  esac
done

if [[ $CLEAN_FLAG == "true" ]]; then
  rm -rf "${SCRIPT_DIR}/build/"
fi

mkdir -p "${SCRIPT_DIR}/build"
mkdir -p "${SCRIPT_DIR}/bin"

echo "=== Configuring CMake (tests enabled) ==="
cd "${SCRIPT_DIR}/build"

CMAKE_ARGS=(
  -DCMAKE_BUILD_TYPE=Debug
  -DTRADINGBOT_BUILD_TESTS=ON
)

if [[ $COVERAGE_FLAG == "true" ]]; then
  CMAKE_ARGS+=(-DTRADINGBOT_ENABLE_COVERAGE=ON)
fi

cmake "${CMAKE_ARGS[@]}" ..

echo "=== Building ==="
if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
else
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
fi
cmake --build . --parallel "${JOBS}"

TEST_BIN="${SCRIPT_DIR}/bin/${TARGET_NAME}"
if [[ ! -x "$TEST_BIN" ]]; then
  echo "Warning: expected test binary not at $TEST_BIN; trying build tree..." >&2
  TEST_BIN="$(find "${SCRIPT_DIR}/build" -type f -name "${TARGET_NAME}" -perm -111 2>/dev/null | head -n 1)"
fi

if [[ $COVERAGE_FLAG == "true" ]]; then
  echo "=== Running coverage ==="
  cmake --build . --target coverage
fi

if [[ $RUN_FLAG == "true" ]]; then
  if [[ -n "$FILTERED_TEST" ]]; then
    if [[ ! -x "$TEST_BIN" ]]; then
      echo "Error: test binary not found (build ${TARGET_NAME} first)." >&2
      exit 1
    fi
    echo "=== Running tests (gtest filter) ==="
    "$TEST_BIN" --gtest_filter="${FILTERED_TEST}"
  elif [[ $VERBOSE_FLAG == "true" ]]; then
    echo "=== Running tests (ctest -V) ==="
    ctest -V --output-on-failure
  else
    echo "=== Running tests (ctest) ==="
    ctest --output-on-failure
  fi
elif [[ $VERBOSE_FLAG == "true" ]]; then
  echo "=== Running tests (ctest -V) ==="
  ctest -V --output-on-failure
fi

echo "=== Done ==="

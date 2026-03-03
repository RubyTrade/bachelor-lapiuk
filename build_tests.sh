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
# Args
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
    r) echo "=== Will Run the Project after the build ===" 
      RUN_FLAG=true
      ;;
    V) echo "=== Will Run the Project Verbose ===" 
      VERBOSE_FLAG=true
      ;;
    *)
      echo "=== Unknown option ==="
      ;;
  esac
done

if [[ $CLEAN_FLAG == "true" ]]; then
  rm -rf "$SCRIPT_DIR/build/"
fi

mkdir -p "$SCRIPT_DIR/build"

echo "=== Configuring CMake (tests enabled) ==="
cd "$SCRIPT_DIR/build"

DEBUG_ARGS="-DCMAKE_BUILD_TYPE=Debug"

COVERAGE_ARGS=""
if [[ $COVERAGE_FLAG == "true" ]]; then
  COVERAGE_ARGS="-DTRADINGBOT_ENABLE_COVERAGE=ON"
fi

cmake ${DEBUG_ARGS} -DTRADINGBOT_BUILD_TESTS=ON ${COVERAGE_ARGS} ..

FILTERED_ARGS=""
if [[ $FILTERED_TEST != "" ]]; then
  FILTERED_ARGS="--gtest_filter=$FILTERED_TEST"
fi

echo "=== Building ==="
cmake --build .

if [[ $COVERAGE_FLAG == "true" ]]; then
  echo "=== Running coverage ==="
  cmake --build . --target coverage
fi
  
if [[ $VERBOSE_FLAG == "true" ]]; then
  echo "=== Running tests verbosely ==="
  ctest -V --output-on-failure
fi

if [[ $RUN_FLAG == "true" && $VERBOSE_FLAG == "false" ]]; then
  echo "=== Running tests ==="
  TEST_BIN=$(find "$SCRIPT_DIR" -type f -name "$TARGET_NAME" | head -n 1)

  "$TEST_BIN" $FILTERED_ARGS
fi

echo "=== Done ==="

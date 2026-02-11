#!/bin/bash

# If any simple command fails, everything stops
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

CLEAN_FLAG=false
RUN_FLAG=false
DEBUG_FLAG=false
# Args
while getopts "crd" opt; do
  case $opt in
    c) echo "=== Clean build ===" 
      CLEAN_FLAG=true
      ;;
    r) echo "=== Will Run the Project after the build ===" 
      RUN_FLAG=true
      ;;
    d) echo "=== Will build in Debug mode ==="
      DEBUG_FLAG=true
      ;;
    *) echo "=== Unknown option ===" ;;
  esac
done


build ()
{
  if [[ $CLEAN_FLAG == "true" ]]; then
    rm -rf $SCRIPT_DIR/build/
  fi  

  mkdir -p $SCRIPT_DIR/build
  mkdir -p $SCRIPT_DIR/bin

  echo "=== Starting CMake build ==="
  cd $SCRIPT_DIR/build
   
  local DEBUG_ARGS=""
  if [[ $DEBUG_FLAG = "true" ]]; then
    DEBUG_ARGS="-DCMAKE_BUILD_TYPE=Debug"
    echo "=== Building in Debug mode ==="
  else
    echo "=== Building in Release mode ==="
  fi

  cmake ${DEBUG_ARGS} ..
  make
  echo "=== Finished CMake build ==="
}


run ()
{
  echo "=== Bot is started ==="
  cd $SCRIPT_DIR

  # first argument is absolute path to .env,
  # to eliminate searching the relative path to it in app
  # second argument is absolute path to binance PK
  ./bin/TradingBot "${SCRIPT_DIR}/.env" "${SCRIPT_DIR}/binance_private.pem"
  echo "=== Bot is stopped ==="
}


# Build the project using CMake
build

# Run the project (Optional)
if [[ $RUN_FLAG == "true" ]]; then
  run
fi

#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

CLEAN=false
RUN_FLAG=false

# Args
while getopts "cr" opt; do
  case $opt in
    c) echo "=== Clean build ===" 
      CLEAN=true
      ;;
    r) echo "=== Will Run the Project after the build ===" 
      RUN_FLAG=true
      ;;
    *) echo "=== Unknown option ===" ;;
  esac
done


build ()
{
  if [[ $CLEAN == "true" ]]; then
    rm -rf $SCRIPT_DIR/build/
  fi  

  mkdir -p $SCRIPT_DIR/build
  mkdir -p $SCRIPT_DIR/bin

  echo "=== Starting CMake build ==="
  cd $SCRIPT_DIR/build
  cmake ..
  make
  echo "=== Finished CMake build ==="
}


run ()
{
  echo "=== Bot is started ==="
  cd $SCRIPT_DIR
  ./bin/TradingBot  
  echo "=== Bot is stopped ==="
}


# Build the project using CMake
build

# Run the project (Optional)
if [[ $RUN_FLAG == "true" ]]; then
  run
fi

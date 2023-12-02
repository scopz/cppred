#!/bin/bash

if ! [ -f "/.dockerenv" ] && ! [ -f "/.dockerinit" ]; then
  echo "# Starting docker build"
  docker run -it --rm -v $PWD:/src pkred-image

else
  cpu_count=$(nproc)

  # Build FreeImage
  (
    cd FreeImage || exit
    make -j "$cpu_count"
  )

  # Build code generator
  (
    cd code_generation || exit
    cmake .
    make -j "$cpu_count"
  )

  # Generate static resources
  (
    cd CodeGeneration || exit
    ../code_generation/code_generation
  )

  # Build game
  (
    cd cppred || exit
    cmake .
    make -j $cpu_count
  )
fi
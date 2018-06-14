#!/usr/bin/env bash

rm -r build

# Run CMake with proper configuration for each HW board
# TODO: accept ninja as optional argument, create a batch script for Windows
#       with ninja as default

for board in mouse keyboard dongle
do
    mkdir -p build/nrf52_desktop_$board

    ( cd build/nrf52_desktop_$board &&
      cmake ../.. -DBOARD=nrf52_desktop_$board )
done

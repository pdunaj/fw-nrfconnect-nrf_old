#!/usr/bin/env bash

rm -r build

# Run CMake with proper configuration for each HW board
# TODO: accept ninja as optional argument, create a batch script for Windows
#       with ninja as default

for board in nrf52_pca20041 nrf52_desktop_keyboard nrf52_desktop_dongle
do
    mkdir -p build/$board

    ( cd build/$board && cmake ../.. -DBOARD=$board )
done

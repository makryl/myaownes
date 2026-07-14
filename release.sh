#!/bin/bash

set -e

cmake --preset Linux-Release
cmake --build build/Linux-Release --target install

cmake --preset MinGW-Release
cmake --build build/MinGW-Release --target install

cmake --preset Emscripten-Release
cmake --build build/Emscripten-Release --target install

find dist -type f -exec ls -lh {} +

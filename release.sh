#!/bin/bash

set -e

cmake --preset Linux-Release -DCMAKE_C_FLAGS="-fprofile-generate" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate"
cmake --build build/Linux-Release --clean-first --target myaownes-pgo

build/Linux-Release/myaownes-pgo external/nes-test-roms

cmake --preset Linux-Release -DCMAKE_C_FLAGS="-fprofile-use -Wno-missing-profile -Wno-coverage-mismatch" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use"
cmake --build build/Linux-Release --clean-first --target install

cmake --preset Windows-Release -DCMAKE_C_FLAGS="-fprofile-generate" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate"
cmake --build build/Windows-Release --clean-first --target myaownes-pgo

wine build/Windows-Release/myaownes-pgo.exe external/nes-test-roms

cmake --preset Windows-Release -DCMAKE_C_FLAGS="-fprofile-use -Wno-missing-profile -Wno-coverage-mismatch" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use"
cmake --build build/Windows-Release --clean-first --target install

cmake --preset Emscripten-Release -DCMAKE_C_FLAGS="-fprofile-generate" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate -sNODERAWFS=1"
cmake --build build/Emscripten-Release --clean-first --target myaownes-pgo

LLVM_PROFILE_FILE=build/Emscripten-Release/myaownes-pgo.profraw node build/Emscripten-Release/myaownes-pgo.js external/nes-test-roms
$EMSDK/upstream/bin/llvm-profdata merge -output=build/Emscripten-Release/myaownes-pgo.profdata build/Emscripten-Release/myaownes-pgo.profraw

cmake --preset Emscripten-Release -DCMAKE_C_FLAGS="-fprofile-use=$PWD/build/Emscripten-Release/myaownes-pgo.profdata" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use=$PWD/build/Emscripten-Release/myaownes-pgo.profdata"
cmake --build build/Emscripten-Release --clean-first --target install

find dist -type f -exec ls -lh {} +

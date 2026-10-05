#!/bin/bash

set -e

cmake --preset Linux-Release -DCMAKE_C_FLAGS="-fprofile-generate" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate"
cmake --build build/Linux-Release --clean-first --target myaownes-pgo-run

cmake --preset Linux-Release -DCMAKE_C_FLAGS="-fprofile-use -Wno-missing-profile -Wno-coverage-mismatch" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use"
cmake --build build/Linux-Release --clean-first --target install

cmake --preset Windows-Release -DCMAKE_C_FLAGS="-fprofile-generate" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate"
cmake --build build/Windows-Release --clean-first --target myaownes-pgo-run

cmake --preset Windows-Release -DCMAKE_C_FLAGS="-fprofile-use -Wno-missing-profile -Wno-coverage-mismatch" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use"
cmake --build build/Windows-Release --clean-first --target install

cmake --preset Emscripten-Release -DCMAKE_C_FLAGS="-fprofile-generate=pgo" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate=pgo -sNODERAWFS=1"
cmake --build build/Emscripten-Release --clean-first --target myaownes-pgo-run

$EMSDK/upstream/bin/llvm-profdata merge -output=build/Emscripten-Release/pgo.profdata build/Emscripten-Release/pgo

cmake --preset Emscripten-Release -DCMAKE_C_FLAGS="-fprofile-use=pgo.profdata" -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use=pgo.profdata"
cmake --build build/Emscripten-Release --clean-first --target install

find dist -type f -exec ls -lh {} +

cmake_minimum_required(VERSION 3.28)

project(myanes_workspace LANGUAGES C)

set(CMAKE_C_STANDARD 23)
add_compile_options(-Wall -Wextra)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
add_subdirectory(external/SDL3-3.4.8 EXCLUDE_FROM_ALL)

include_directories(include)

add_library(myanes
  src/apu.c
  src/cpu.c
  src/dev.c
  src/map.c
  src/ppu.c
  src/rom.c
)
target_link_libraries(myanes PUBLIC GLESv2 SDL3-static)

add_executable(tests src/tests.c)
target_link_libraries(tests myanes)

function (target_emscripten name html js)
  set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_NAME ${name}
    SUFFIX .html
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${html};${CMAKE_CURRENT_SOURCE_DIR}/${js}"
  )
  target_link_options(${name} PRIVATE
    --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/${html}
    --pre-js=${CMAKE_CURRENT_SOURCE_DIR}/${js}
  )
  install(
    TARGETS ${name}
    DESTINATION "./"
    # FILES
    #   $<$<CONFIG:Debug>:${CMAKE_CURRENT_BINARY_DIR}/index.js>
    #   $<$<CONFIG:Debug>:${CMAKE_CURRENT_BINARY_DIR}/index.wasm>
    #   $<$<CONFIG:Debug>:${CMAKE_CURRENT_BINARY_DIR}/index.wasm.map>
    # DESTINATION "./"
  )
endfunction()

add_executable(myanestest src/test.c)
target_link_libraries(myanestest myanes)
target_emscripten(myanestest src/index.html src/index.js)

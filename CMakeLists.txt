cmake_minimum_required(VERSION 3.28)

project(myaownes_workspace LANGUAGES C)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
add_subdirectory(external/SDL-release-3.4.16 EXCLUDE_FROM_ALL)

set(CMAKE_C_STANDARD 23)
add_compile_options(-Wall -Wextra)

add_library(myaownes-static
  src/apu.c
  src/cpu.c
  src/dev.c
  src/map.c
  src/ppu.c
)
target_include_directories(myaownes-static PUBLIC include)
target_link_libraries(myaownes-static PUBLIC SDL3-static)

add_executable(myaownes src/main.c)
target_link_libraries(myaownes myaownes-static)

if (EMSCRIPTEN)
  em_add_link_deps(myaownes --pre-js src/index.js)
  em_add_link_deps(myaownes --shell-file src/index.html)
endif()

install(TARGETS myaownes)

add_executable(myaownes-tests EXCLUDE_FROM_ALL src/tests.c)
target_link_libraries(myaownes-tests myaownes-static)

add_custom_target(myaownes-tests-run
  COMMAND myaownes-tests "${TESTS_BASE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/" > "${CMAKE_CURRENT_SOURCE_DIR}/tests/log.txt"
  DEPENDS myaownes-tests
)

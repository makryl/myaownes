cmake_minimum_required(VERSION 3.28)

project(myaownes LANGUAGES C)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
add_subdirectory(${SDL_DIR} EXCLUDE_FROM_ALL)

set(CMAKE_C_STANDARD 23)
add_compile_options(-Wall -Wextra)

add_library(${PROJECT_NAME}-static
  src/apu.c
  src/cpu.c
  src/dev.c
  src/map.c
  src/ppu.c
)
target_include_directories(${PROJECT_NAME}-static PUBLIC include)
target_link_libraries(${PROJECT_NAME}-static PUBLIC SDL3-static)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-static)

add_executable(${PROJECT_NAME}-tests EXCLUDE_FROM_ALL src/tests.c)
target_link_libraries(${PROJECT_NAME}-tests ${PROJECT_NAME}-static)
add_custom_target(${PROJECT_NAME}-tests-run COMMAND ${PROJECT_NAME}-tests "${TEST_ROMS_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/" > "${CMAKE_CURRENT_SOURCE_DIR}/tests/log.txt")

add_executable(${PROJECT_NAME}-pgo EXCLUDE_FROM_ALL src/pgo.c)
target_link_libraries(${PROJECT_NAME}-pgo ${PROJECT_NAME}-static)
add_custom_target(${PROJECT_NAME}-pgo-run COMMAND ${PROJECT_NAME}-pgo "${TEST_ROMS_DIR}")

install(TARGETS ${PROJECT_NAME} DESTINATION "./")

if (EMSCRIPTEN)
  em_add_link_deps(${PROJECT_NAME} --pre-js src/index.js)
  em_add_link_deps(${PROJECT_NAME} --shell-file src/index.html)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_NAME index
    SUFFIX .html
  )
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/index.js
    ${CMAKE_CURRENT_BINARY_DIR}/index.wasm
    ${CMAKE_CURRENT_BINARY_DIR}/index.wasm.map
    DESTINATION "./"
    OPTIONAL
  )
endif()

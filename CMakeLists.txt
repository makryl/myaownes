cmake_minimum_required(VERSION 3.28)

project(myanes_workspace LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra)

include_directories(include)

add_library(myanes
  src/nes.c
  src/dev.c
  src/cpu.c
  src/ppu.c
  src/apu.c
)

add_executable(myanestest src/myanestest.c)
target_link_libraries(myanestest myanes)

# install(TARGETS sandbox RUNTIME DESTINATION "./")


# function target_emscripten(name html js)
#   set_target_properties(${name} PROPERTIES
#     RUNTIME_OUTPUT_NAME ${name}
#     SUFFIX .html
#     LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${html};${CMAKE_CURRENT_SOURCE_DIR}/${js}"
#   )
#   target_link_options(${name} PRIVATE
#     --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/${html}
#     --pre-js=${CMAKE_CURRENT_SOURCE_DIR}/${js}
#   )
# endfunction()

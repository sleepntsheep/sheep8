cmake_minimum_required(VERSION 3.10)

set(TARGET_NAME sheep8)

# set the project name
project(${TARGET_NAME} VERSION 1.0 LANGUAGES C)

set(SOURCE_FILE
    src/main.c 
    src/chip8.c
    )

# add the executable
add_executable(${TARGET_NAME} ${SOURCE_FILE})

# c standard
set_property(TARGET ${TARGET_NAME} PROPERTY C_STANDARD 90)

# SDL
find_package(SDL2 REQUIRED)

# include path
target_include_directories(${TARGET_NAME} PUBLIC ${sheep8_SOURCE_DIR}/include)

# mingw
if (MINGW) 
    target_link_libraries(${TARGET_NAME} PRIVATE mingw32 SDL2main ssp)
    target_compile_options(${TARGET_NAME} PRIVATE)
endif()

# library path
target_link_libraries(${TARGET_NAME} PRIVATE ${sheep8_SOURCE_DIR}/lib)

# math
target_link_libraries(${TARGET_NAME} PRIVATE SDL2 m)

# macro 
target_compile_definitions(${TARGET_NAME}
  PRIVATE
  INTERFACE
)

# wall, werror
if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-long-long)
endif()

# output dir
set_target_properties( ${TARGET_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${sheep8_SOURCE_DIR}/bin"
)

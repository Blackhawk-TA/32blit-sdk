cmake_minimum_required(VERSION 3.9)

# has to be before project
include(32blit-pico/sdk_import.cmake)

project(32blit)

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4244")
endif()

find_package (32BLIT CONFIG REQUIRED PATHS .)
add_subdirectory(examples)

add_subdirectory(launcher-shared)

if(32BLIT_HW)
    add_subdirectory(32blit-stm32)
    add_subdirectory(firmware)
    add_subdirectory(firmware-update)
endif()

if(NOT 32BLIT_PICO)
    add_subdirectory(launcher)
endif()

# include dist files in install
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/dist/
    DESTINATION .
)

# setup release packages
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)

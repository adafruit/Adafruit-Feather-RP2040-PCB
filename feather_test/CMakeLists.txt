# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "C:/Users/ladyada/Dropbox/pico/pico-sdk")
set(PICO_DEFAULT_BOOT_STAGE2_FILE, "foo")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(feather_test C CXX ASM)

# Initialise the Pico SDK
pico_sdk_init()


# Add executable. Default name is the project name, version 0.1

add_executable(feather_test feather_test.c )

pico_set_program_name(feather_test "feather_test")
pico_set_program_version(feather_test "0.1")

pico_enable_stdio_uart(feather_test 1)
pico_enable_stdio_usb(feather_test 0)

set(PICO_DEFAULT_BOOT_STAGE2_FILE "${CMAKE_CURRENT_LIST_DIR}/boot2_w25qs080.S")


# Add the standard library to the build
target_link_libraries(feather_test pico_stdlib hardware_adc hardware_pio hardware_dma)

pico_add_extra_outputs(feather_test)


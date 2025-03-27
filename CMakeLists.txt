# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Testesw C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(Testesw SMILE.c
saveSystem.c
 )

add_subdirectory("C:/Users/juuze/Documents/Projeto\ Embarca/Testesw/pico-ssd1306")

pico_set_program_name(Testesw "Testesw")
pico_set_program_version(Testesw "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Testesw 0)
pico_enable_stdio_usb(Testesw 1)
include_directories("${PICO_SDK_PATH}/lib/lwip/src/include")

# Add the standard library to the build
target_link_libraries(Testesw
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        hardware_i2c
        hardware_adc
        pico-ssd1306
        hardware_flash 
        hardware_sync
        hardware_timer)

# Add the standard include files to the build
target_include_directories(Testesw PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/pico-ssd1306

)


# Add any user requested libraries
target_link_libraries(Testesw 
        
        )

pico_add_extra_outputs(Testesw)


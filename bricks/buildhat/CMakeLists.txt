cmake_minimum_required(VERSION 3.13)

get_filename_component(TOP_DIR "../.." ABSOLUTE)
get_filename_component(PBIO_DIR "../../lib/pbio" ABSOLUTE)
set(PBIO_PLATFORM_DIR ${PBIO_DIR}/platform/build_hat)

set(PICO_BOARD rpi_build_hat)
set(PICO_PLATFORM rp2040)
set(PICO_BOARD_HEADER_DIRS ${PBIO_PLATFORM_DIR})

set(PICO_NO_FLASH ON)

# Make lists passed from calling Makefile absolute.
list(TRANSFORM PYBRICKS_PYBRICKS_SRC_C PREPEND ${TOP_DIR}/)
list(TRANSFORM PYBRICKS_LIB_SRC_C PREPEND ${TOP_DIR}/)

list(APPEND PYBRICKS_PYBRICKS_SRC_C
    ${TOP_DIR}/bricks/_common/micropython.c
    ${TOP_DIR}/bricks/_common/mphalport.c
)

# TODO: This is a hack that will go away eventually.
list(APPEND PYBRICKS_LIB_SRC_C
    ${PBIO_DIR}/drv/usb/usb_simulation_pico.c
)

# Set build type to reduce firmware size
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

# Set main target and component locations
set(MICROPY_TARGET firmware)
get_filename_component(MICROPY_DIR "../../micropython" ABSOLUTE)
set(PICO_SDK_PATH ${MICROPY_DIR}/lib/pico-sdk)

# Set the location of MicroPython RP2 port directory.
set(MICROPY_RP2_PORT_DIR ${MICROPY_DIR}/ports/rp2)

set(MICROPY_BOARD RPI_BUILD_HAT)

# Pybricks doesn't differentiate between port and board directories.
set(MICROPY_BOARD_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MICROPY_PORT_DIR ${MICROPY_BOARD_DIR})

set(MICROPY_USER_FROZEN_MANIFEST ${MICROPY_FROZEN_MANIFEST})

string(TOLOWER ${MICROPY_BOARD} PICO_BOARD)

# Set the amount of C heap, if it's not already set.
# If a board uses malloc then it must set this to at least 4096.
if(NOT MICROPY_C_HEAP_SIZE)
    set(MICROPY_C_HEAP_SIZE 0)
endif()

# Enable error text compression by default.
if(NOT MICROPY_ROM_TEXT_COMPRESSION)
    set(MICROPY_ROM_TEXT_COMPRESSION ON)
endif()

# Workaround for pico-sdk host toolchain issue, see directory for details
list(APPEND CMAKE_MODULE_PATH "${MICROPY_PORT_DIR}/tools_patch")

# Include component cmake fragments
include(${MICROPY_DIR}/py/py.cmake)
include(${MICROPY_DIR}/extmod/extmod.cmake)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Define the top-level project
project(${MICROPY_TARGET})

pico_sdk_init()

include(${MICROPY_DIR}/py/usermod.cmake)

add_executable(${MICROPY_TARGET})

# Provide a C-level definitions of PICO_ARM.
# (The pico-sdk already defines PICO_RISCV when it's enabled.)
if(PICO_ARM)
    target_compile_definitions(pico_platform_headers INTERFACE
        PICO_ARM=1
    )
endif()

set(MICROPY_SOURCE_LIB
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/runtime/gchelper_native.c
    ${MICROPY_DIR}/shared/runtime/interrupt_char.c
    ${MICROPY_DIR}/shared/runtime/pyexec.c
    ${MICROPY_DIR}/shared/runtime/stdout_helpers.c
    ${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c
)

if(PICO_ARM)
    list(APPEND MICROPY_SOURCE_LIB
        ${MICROPY_DIR}/shared/runtime/gchelper_thumb1.s
    )
elseif(PICO_RISCV)
    list(APPEND MICROPY_SOURCE_LIB
        ${MICROPY_DIR}/shared/runtime/gchelper_rv32i.s
    )
endif()

set(MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_PY}
    ${MICROPY_DIR}/shared/readline/readline.c
    ${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c
)

set(PICO_SDK_COMPONENTS
    boot_bootrom_headers
    hardware_adc
    hardware_base
    hardware_boot_lock
    hardware_clocks
    hardware_dma
    hardware_flash
    hardware_gpio
    hardware_i2c
    hardware_irq
    hardware_pio
    hardware_pll
    hardware_pwm
    hardware_regs
    hardware_resets
    hardware_spi
    hardware_structs
    hardware_sync
    hardware_sync_spin_lock
    hardware_timer
    hardware_uart
    hardware_watchdog
    hardware_xosc
    pico_aon_timer
    pico_base_headers
    pico_binary_info
    pico_bootrom
    pico_flash
    pico_multicore
    pico_platform
    pico_platform_compiler
    pico_platform_panic
    pico_platform_sections
    pico_runtime
    pico_runtime_init
    pico_stdio
    pico_stdlib
    pico_sync
    pico_time
    pico_unique_id
    pico_util
)

if(PICO_ARM)
    list(APPEND PICO_SDK_COMPONENTS
        cmsis_core
    )
elseif(PICO_RISCV)
    list(APPEND PICO_SDK_COMPONENTS
        hardware_hazard3
        hardware_riscv
    )
endif()

# Use our custom pico_float_micropython float implementation.  This is needed for two reasons:
# - to fix inf handling in pico-sdk's __wrap___aeabi_fadd();
# - so we can use our own libm functions, to fix inaccuracies in the pico-sdk versions.
pico_set_float_implementation(${MICROPY_TARGET} micropython)

# Define our custom pico_float_micropython component.
pico_add_library(pico_float_micropython)

# pico_float_micropython: add pico-sdk float and our libm source files.
target_sources(pico_float_micropython INTERFACE
    ${MICROPY_SOURCE_LIB_LIBM}
    ${MICROPY_SOURCE_LIB_LIBM_SQRT_SW}
    ${MICROPY_RP2_PORT_DIR}/libm_extra.c
)

target_sources(pico_float_micropython INTERFACE
    ${PICO_SDK_PATH}/src/rp2_common/pico_float/float_aeabi_rp2040.S
    ${PICO_SDK_PATH}/src/rp2_common/pico_float/float_init_rom_rp2040.c
    ${PICO_SDK_PATH}/src/rp2_common/pico_float/float_v1_rom_shim_rp2040.S
)

# pico_float_micropython: wrap low-level floating-point ops, to call the pico-sdk versions.
pico_wrap_function(pico_float_micropython __aeabi_fdiv)
pico_wrap_function(pico_float_micropython __aeabi_fmul)
pico_wrap_function(pico_float_micropython __aeabi_frsub)
pico_wrap_function(pico_float_micropython __aeabi_fsub)
pico_wrap_function(pico_float_micropython __aeabi_cfcmpeq)
pico_wrap_function(pico_float_micropython __aeabi_cfrcmple)
pico_wrap_function(pico_float_micropython __aeabi_cfcmple)
pico_wrap_function(pico_float_micropython __aeabi_fcmpeq)
pico_wrap_function(pico_float_micropython __aeabi_fcmplt)
pico_wrap_function(pico_float_micropython __aeabi_fcmple)
pico_wrap_function(pico_float_micropython __aeabi_fcmpge)
pico_wrap_function(pico_float_micropython __aeabi_fcmpgt)
pico_wrap_function(pico_float_micropython __aeabi_fcmpun)
pico_wrap_function(pico_float_micropython __aeabi_i2f)
pico_wrap_function(pico_float_micropython __aeabi_l2f)
pico_wrap_function(pico_float_micropython __aeabi_ui2f)
pico_wrap_function(pico_float_micropython __aeabi_ul2f)
pico_wrap_function(pico_float_micropython __aeabi_f2iz)
pico_wrap_function(pico_float_micropython __aeabi_f2lz)
pico_wrap_function(pico_float_micropython __aeabi_f2uiz)
pico_wrap_function(pico_float_micropython __aeabi_f2ulz)
pico_wrap_function(pico_float_micropython __aeabi_f2d)


# Add qstr sources for extmod and usermod, in case they are modified by components above.
list(APPEND MICROPY_SOURCE_QSTR
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_USERMOD}
    ${MICROPY_SOURCE_BOARD}
    ${PYBRICKS_PYBRICKS_SRC_C}
)

# Define mpy-cross flags
set(MICROPY_CROSS_FLAGS -march=armv6m)

# Set the frozen manifest file
if (MICROPY_USER_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${MICROPY_USER_FROZEN_MANIFEST})
endif()

target_sources(${MICROPY_TARGET} PRIVATE
    ${MICROPY_SOURCE_PY}
    ${MICROPY_SOURCE_EXTMOD}
    ${MICROPY_SOURCE_LIB}
    ${MICROPY_SOURCE_DRIVERS}
    ${MICROPY_SOURCE_PORT}
    ${MICROPY_SOURCE_BOARD}
    ${PYBRICKS_PYBRICKS_SRC_C}
    ${PYBRICKS_LIB_SRC_C}
)

target_link_libraries(${MICROPY_TARGET} usermod)

target_include_directories(${MICROPY_TARGET} PRIVATE
    ${PBIO_PLATFORM_DIR}
    ${PBIO_DIR}/include
    ${TOP_DIR}/lib/lego
    ${TOP_DIR}/lib/lwrb/src/include
    ${TOP_DIR}
    ${MICROPY_INC_CORE}
    ${MICROPY_INC_USERMOD}
    ${MICROPY_BOARD_DIR}
    "${MICROPY_PORT_DIR}"
    "${CMAKE_BINARY_DIR}"
)

target_compile_options(${MICROPY_TARGET} PRIVATE
    -Wall
    -Werror
    -g  # always include debug information in the ELF
)

# Apply optimisations to performance-critical source code.
set_source_files_properties(
    ${MICROPY_PY_DIR}/map.c
    ${MICROPY_PY_DIR}/mpz.c
    ${MICROPY_PY_DIR}/vm.c
    PROPERTIES
    COMPILE_OPTIONS "-O2"
)

set_source_files_properties(
    ${PICO_SDK_PATH}/src/rp2_common/pico_double/double_math.c
    ${PICO_SDK_PATH}/src/rp2_common/pico_float/float_math.c
    PROPERTIES
    COMPILE_OPTIONS "-Wno-error=uninitialized"
)

target_compile_definitions(${MICROPY_TARGET} PRIVATE
    ${MICROPY_DEF_BOARD}
    PICO_FLOAT_PROPAGATE_NANS=1
    PICO_STACK_SIZE=0x1000 # Matches available scratch size in linker script.
    PICO_MAX_SHARED_IRQ_HANDLERS=8 # we need more than the default
    PICO_PROGRAM_NAME="Pybricks MicroPython"
    PICO_NO_PROGRAM_VERSION_STRING=1 # do it ourselves in main.c
    MICROPY_BUILD_TYPE="${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} ${CMAKE_BUILD_TYPE}"
    PICO_NO_BI_STDIO_UART=1 # uart0 uses Pybricks Profile instead
)

target_link_libraries(${MICROPY_TARGET}
    ${PICO_SDK_COMPONENTS}
)

if (MICROPY_HW_ENABLE_DOUBLE_TAP)
# Enable double tap reset into bootrom.
target_link_libraries(${MICROPY_TARGET}
    pico_bootsel_via_double_reset
)
endif()

pico_set_linker_script(${MICROPY_TARGET} ${PBIO_PLATFORM_DIR}/rpi_build_hat.ld)

pico_add_extra_outputs(${MICROPY_TARGET})

pico_find_compiler_with_triples(PICO_COMPILER_SIZE "${PICO_GCC_TRIPLE}" size)

add_custom_command(TARGET ${MICROPY_TARGET}
    POST_BUILD
    COMMAND ${PICO_COMPILER_SIZE} --format=berkeley ${PROJECT_BINARY_DIR}/${MICROPY_TARGET}.elf
    VERBATIM
)

# Collect all the include directories and compile definitions for the pico-sdk components.
foreach(comp ${PICO_SDK_COMPONENTS})
    micropy_gather_target_properties(${comp})
    micropy_gather_target_properties(${comp}_headers)
endforeach()

set(MICROPY_CPP_FLAGS_EXTRA ${PICO_COMMON_LANG_FLAGS})
separate_arguments(MICROPY_CPP_FLAGS_EXTRA)

# Include the main MicroPython cmake rules.
include(${MICROPY_DIR}/py/mkrules.cmake)

if(NOT PICO_NUM_GPIOS)
    set(PICO_NUM_GPIOS 30)
endif()

if(NOT PICO_NUM_EXT_GPIOS)
    set(PICO_NUM_EXT_GPIOS 10)
endif()

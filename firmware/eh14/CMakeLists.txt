cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

find_program(ARM_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_GXX arm-none-eabi-g++ REQUIRED)
find_program(ARM_ASM arm-none-eabi-gcc REQUIRED)
# gcc-ar/gcc-ranlib load the LTO plugin, which plain ar/ranlib do not. Without them a static
# library built with -flto archives fine but links with "undefined reference".
find_program(ARM_AR arm-none-eabi-gcc-ar REQUIRED)
find_program(ARM_RANLIB arm-none-eabi-gcc-ranlib REQUIRED)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(ARM_SIZE arm-none-eabi-size REQUIRED)

set(CMAKE_C_COMPILER ${ARM_GCC})
set(CMAKE_CXX_COMPILER ${ARM_GXX})
set(CMAKE_ASM_COMPILER ${ARM_ASM})
set(CMAKE_AR ${ARM_AR})
set(CMAKE_RANLIB ${ARM_RANLIB})
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(eh14 C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# The EH14 main board carries an ATSAMD21G18A: Cortex-M0+, 256K of flash, 32K of RAM, 32.768kHz crystal.
set(MCU_DEFINE "__SAMD21G18A__")
set(MCU_LOWER "atsamd21g18a")
set(JLINK_DEVICE "ATSAMD21G18A")

set(CPU_FLAGS
  -mcpu=cortex-m0plus
  -mthumb
  -mfloat-abi=soft
)

set(COMMON_WARNINGS
  -Wall
  -Wextra
  -Wpedantic
)

# -O2: there is flash to spare, and the code that has to keep time does it against the clock, not by
# counting instructions (see SegmentDriver for the one place that waits a few cycles).
set(OPTIMIZATION
  $<$<CONFIG:Debug>:-Og>
  $<$<NOT:$<CONFIG:Debug>>:-O2>
)

# ---------------------------------------------------------------------------
# Memory layout
#
# Flash: the UF2 bootloader (bootloader/eh14-bootloader.hex) takes the first 8K and protects them (the
# BOOTPROT fuse), and starts the application at 0x2000 - the rest of the 256K is the application's.
#
# RAM: all of it, but the top word. The bootloader looks there after a reset for its "stay in the bootloader"
# magic (UsbSerial's bootloader request writes it), so the stack starts just below it and never touches it.
# ---------------------------------------------------------------------------
set(LD_FLASH_ORIGIN 0x00002000)
set(LD_FLASH_LENGTH 248K)
set(LD_RAM_ORIGIN 0x20000000)
set(LD_RAM_LENGTH 0x7FFC)

set(LINKER_SCRIPT "${CMAKE_BINARY_DIR}/${MCU_LOWER}.ld")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/ld/${MCU_LOWER}.ld.in"
  "${LINKER_SCRIPT}"
  @ONLY
)

set(COMMON_DEFINES
  ${MCU_DEFINE}
  DONT_USE_CMSIS_INIT
)

# ---------------------------------------------------------------------------
# Libraries (git submodules in lib/)
# ---------------------------------------------------------------------------
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMSIS_DIR "${LIB_DIR}/CMSIS_6")
set(SAMD21_DIR "${LIB_DIR}/microchip_driver/samd21")
set(TINYUSB_DIR "${LIB_DIR}/tinyusb")
set(UF2_DIR "${LIB_DIR}/uf2")

foreach(required_file
  "${CMSIS_DIR}/CMSIS/Core/Include/core_cm0plus.h"
  "${SAMD21_DIR}/include/samd21g18a.h"
  "${TINYUSB_DIR}/src/tusb.c"
  "${UF2_DIR}/utils/uf2conv.py")
  if(NOT EXISTS "${required_file}")
    message(FATAL_ERROR "Missing ${required_file}\n"
                        "A submodule is missing, fetch it with:\n"
                        "  git submodule update --init --recursive")
  endif()
endforeach()

set(APP_SOURCES
  src/main.cpp
  src/Eh14.cpp
  src/app/Application.cpp
  src/app/Menu.cpp
  src/app/SerialProtocol.cpp
  src/app/Settings.cpp
  src/app/SoundBank.cpp
  src/app/TestMode.cpp
  src/app/TimeEditor.cpp
  src/app/Voice.cpp
  src/hardware/AnalogInputs.cpp
  src/hardware/AudioOut.cpp
  src/hardware/Buttons.cpp
  src/hardware/Clocks.cpp
  src/hardware/Display.cpp
  src/hardware/ExternalInterrupts.cpp
  src/hardware/I2cBus.cpp
  src/hardware/IrReceiver.cpp
  src/hardware/RealTimeClock.cpp
  src/hardware/SegmentDriver.cpp
  src/hardware/Sleep.cpp
  src/hardware/SpiBus.cpp
  src/hardware/SpiFlash.cpp
  src/hardware/StatusLed.cpp
  src/hardware/Timebase.cpp
  src/usb/UsbSerial.cpp
  src/usb/usb_descriptors.c
  src/startup_samd21.c
)

set(COMMON_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMSIS_DIR}/CMSIS/Core/Include
  ${SAMD21_DIR}/include
)

# Flags for third party code: the CPU and the optimization, none of the app's warnings.
set(LIBRARY_OPTIONS
  ${CPU_FLAGS}
  -ffunction-sections
  -fdata-sections
  -fno-common
  ${OPTIMIZATION}
  -g3
  -fno-unwind-tables
  -fno-asynchronous-unwind-tables
  -flto=auto
)

# ---------------------------------------------------------------------------
# TinyUSB device stack (hathach, MIT): a CDC serial port on the SAMD21's USB controller.
#
# Built as its own library rather than folded into the app: it is third party code that neither wants the
# app's warning flags nor should be held to them.
# ---------------------------------------------------------------------------
set(TINYUSB_SOURCES
  ${TINYUSB_DIR}/src/tusb.c
  ${TINYUSB_DIR}/src/common/tusb_fifo.c
  ${TINYUSB_DIR}/src/device/usbd.c
  ${TINYUSB_DIR}/src/class/cdc/cdc_device.c
  ${TINYUSB_DIR}/src/portable/microchip/samd/dcd_samd.c
)

add_library(tinyusb STATIC ${TINYUSB_SOURCES})

target_include_directories(tinyusb PUBLIC
  ${TINYUSB_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb
)

target_include_directories(tinyusb PRIVATE ${COMMON_INCLUDES})
target_compile_definitions(tinyusb PUBLIC ${COMMON_DEFINES})
target_compile_options(tinyusb PRIVATE ${LIBRARY_OPTIONS})

# ---------------------------------------------------------------------------
# Firmware
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME}.elf ${APP_SOURCES})

target_link_libraries(${PROJECT_NAME}.elf PRIVATE tinyusb)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
  SUFFIX ".elf"
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${COMMON_INCLUDES})
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE ${COMMON_DEFINES} NDEBUG)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
  ${CPU_FLAGS}
  ${COMMON_WARNINGS}
  -ffunction-sections
  -fdata-sections
  -fno-common
  ${OPTIMIZATION}
  -g3
  -fno-unwind-tables
  -fno-asynchronous-unwind-tables
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
  $<$<COMPILE_LANGUAGE:CXX>:-Wdouble-promotion>
  -flto=auto
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
  ${CPU_FLAGS}
  -specs=nano.specs
  -specs=nosys.specs
  -nostartfiles
  -T${LINKER_SCRIPT}
  -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
  -Wl,--gc-sections
  -Wl,--print-memory-usage
  -flto=auto
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 12)
  target_link_options(${PROJECT_NAME}.elf PRIVATE -Wl,--no-warn-rwx-segments)
endif()

# The Microchip device headers are not -Wpedantic clean (anonymous structs in every register union), and
# neither are the TinyUSB headers the USB sources pull in.
set_source_files_properties(src/usb/usb_descriptors.c src/startup_samd21.c
  PROPERTIES COMPILE_OPTIONS "-Wno-pedantic")

# TinyUSB's OS shim increments a volatile counter, which C++20 deprecated and C did not - so the warning only
# appears where a C++ file includes tusb.h.
set_source_files_properties(src/usb/UsbSerial.cpp
  PROPERTIES COMPILE_OPTIONS "-Wno-volatile")

# The register unions of the device headers are anonymous structs, a GNU extension -Wpedantic complains about
# in every C++ file that includes sam.h.
target_compile_options(${PROJECT_NAME}.elf PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-pedantic>)

# ---------------------------------------------------------------------------
# Output images
# ---------------------------------------------------------------------------
set(HEX_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex")
set(BIN_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin")
set(UF2_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.uf2")

# The bootloader takes a file of the SAMD21 family only.
set(UF2_FAMILY_ID 0x68ed2b88)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
  COMMAND ${ARM_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
  COMMAND ${ARM_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
  COMMAND ${Python3_EXECUTABLE} ${UF2_DIR}/utils/uf2conv.py -c -f ${UF2_FAMILY_ID} -b ${LD_FLASH_ORIGIN}
    -o ${UF2_FILE} ${BIN_FILE} > /dev/null
  COMMAND ${ARM_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
  COMMENT "Generating HEX, BIN and UF2 images"
)

# ---------------------------------------------------------------------------
# Flashing
#
# With J-Link the application alone (flash-jlink), the bootloader alone (flash-bootloader-jlink), or both
# (flash-all-jlink). Every script first makes the display safe (misc/jlink-safe-display.in), see CLAUDE.md.
# Without a probe, flash-usb asks the running firmware for the bootloader over USB and copies the UF2 file
# onto its drive.
# ---------------------------------------------------------------------------
set(JLINKEXE "JLinkExe" CACHE FILEPATH "Path to SEGGER JLinkExe")
set(BOOTLOADER_HEX "${CMAKE_CURRENT_SOURCE_DIR}/bootloader/eh14-bootloader.hex")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/misc/jlink-safe-display.in JLINK_SAFE_DISPLAY)

# A J-Link target running misc/<SCRIPT>.in, a Commander script with the image paths filled in.
function(add_jlink_target NAME SCRIPT DESCRIPTION)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/${SCRIPT}.in ${CMAKE_BINARY_DIR}/${SCRIPT}.jlink @ONLY)
  add_custom_target(${NAME}
    COMMAND ${JLINKEXE}
      -noGui 1
      -device ${JLINK_DEVICE}
      -if swd
      -speed 4000
      -autoconnect 1
      -CommanderScript ${CMAKE_BINARY_DIR}/${SCRIPT}.jlink
    ${ARGN}
    COMMENT ${DESCRIPTION}
  )
endfunction()

add_jlink_target(flash-jlink jlink-flash "Flashing ${PROJECT_NAME}.hex with J-Link"
  DEPENDS ${PROJECT_NAME}.elf)
add_jlink_target(flash-all-jlink jlink-flash-all "Flashing the bootloader and ${PROJECT_NAME}.hex with J-Link"
  DEPENDS ${PROJECT_NAME}.elf)
add_jlink_target(flash-bootloader-jlink jlink-flash-bootloader "Flashing the bootloader with J-Link")

# The clock has to be awake (its menu open), or USB is not there to ask.
add_custom_target(flash-usb
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/flash_usb.py ${UF2_FILE}
  DEPENDS ${PROJECT_NAME}.elf
  USES_TERMINAL
  COMMENT "Copying ${PROJECT_NAME}.uf2 onto the bootloader's drive"
)

# Sets the clock to this computer's time - the clock awake, its menu open.
add_custom_target(set-time
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/set_time.py
  USES_TERMINAL
  COMMENT "Setting the clock to this computer's time"
)

message(STATUS "Device:        ${JLINK_DEVICE}")
message(STATUS "App flash:     ${LD_FLASH_ORIGIN} + ${LD_FLASH_LENGTH}")
message(STATUS "Bootloader:    ${BOOTLOADER_HEX}")
message(STATUS "Linker script: ${LINKER_SCRIPT}")

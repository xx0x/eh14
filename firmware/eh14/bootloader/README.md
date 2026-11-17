# Bootloader

`eh14-bootloader.hex` is based on the Adafruit UF2 bootloader for the Feather M0, available from
[uf2-samdx1](https://github.com/adafruit/uf2-samdx1).

It takes the first 8K of the flash (`0x0000-0x1FFF`) and starts the firmware at `0x2000`. The fuses guard it
(BOOTPROT = 8K) and leave the rest unlocked: the user row reads `0xD8E0C7FA 0xFFFFFC5D`.

## Getting into it

- **From the running firmware**, the Arduino IDE's way: open the clock's USB serial port at 1200
  baud and close it again. The firmware leaves the bootloader's magic word at the top of RAM and resets
  (UsbSerial). The clock has to be awake for that - with its menu open.
- **With the RESET button** (newer EH14 clocks): press it twice quickly.

The clock then shows up as a USB flash drive: copying a `.uf2` file onto it installs the new firmware
(`make flash-usb` does all of this). The bootloader also answers `bossac` on its own serial port.

## Flashing it

`make flash-bootloader-jlink` writes it with a J-Link (`make flash-all-jlink` writes the bootloader and the firmware).
With BOOTPROT set, the chip refuses to erase or write those 8K - clear the fuse first (OpenOCD with the J-Link:
`at91samd bootloader 0`), write the bootloader, and set it back (`at91samd bootloader 8192`), so a firmware update can
never touch it. The fuse only takes effect after a reset, and after that reset OpenOCD could not probe the flash in
the same session - so it takes two sessions: one to clear BOOTPROT and reset, a second to write the bootloader and set
BOOTPROT back. Check the user row afterwards: it must read as above.

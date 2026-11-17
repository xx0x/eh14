# EH14 firmware

The main firmware for the Časotron EH14 clock: a new version from 2026, rewritten from the original Arduino sketch (`firmware/legacy/eh14`). From the user's perspective, the clock behaves nearly identically with the old and the new firmware. The new firmware brings bug fixes and long-term maintainability.

## Build

```sh
git submodule update --init --recursive   # once, after a clone
cmake -S . -B build && cmake --build build -j8
```

or run the VS Code task **EH14: Build** (`.vscode/tasks.json`). The build
makes `build/eh14.hex`, `.bin` and `.uf2`. A new `.cpp` goes into `APP_SOURCES` in `CMakeLists.txt`.

**EH14: Publish build** (the `publish` target) builds and copies `eh14.hex` and `eh14.uf2` into `firmware/builds` as
`eh14_app_<YYYYMMDD>.hex` and `.uf2`, dated the day it runs.

### Libraries

Git submodules in `lib/`:

- `CMSIS_6`: the Cortex-M0+ core headers.
- `microchip_driver`: the SAMD21 device headers - only `samd21/include` is used. The startup code and the linker script are the firmware's own (`src/startup_samd21.c`,
  `ld/`).
- `tinyusb`: the USB device stack, CDC only.
- `uf2`: `uf2conv.py`, which makes the `.uf2`.

## Memory layout and bootloader

The UF2 bootloader every EH14 carries (`bootloader/`, see its README) takes the first 8K, guarded by the BOOTPROT fuse,
and starts the firmware at `0x2000`. The top word of RAM is left out of the stack: the bootloader looks for its "stay"
magic there after a reset (`Eh14::EnterBootloader`).

## Flashing

| VS Code task                               | make target              |                                                        |
| ------------------------------------------ | ------------------------ | ------------------------------------------------------ |
| EH14: Flash (J-Link)                       | `flash-jlink`            | the firmware; the bootloader stays                     |
| EH14: Flash bootloader + firmware (J-Link) | `flash-all-jlink`        | clear BOOTPROT first (see `bootloader/README.md`)      |
| EH14: Flash bootloader (J-Link)            | `flash-bootloader-jlink` | likewise                                               |
| EH14: Flash over USB                       | `flash-usb`              | no probe needed; the clock must be awake (menu open)   |
| EH14: Set time                             | `set-time`               | this computer's time; the clock must be awake          |

`flash-usb` (`tools/flash_usb.py`) is the way for anyone without a probe: it opens the clock's serial port at 1200 baud
and closes it, the firmware hands over to the bootloader, and the UF2 file goes onto its USB mass storage drive. Newer boards
have a RESET button, which does the same when pressed twice.

**Every J-Link script starts with `misc/jlink-safe-display.in`:** it halts the core, switches the boost converter and
the amplifier off and clears the display's shift registers. A halt can land in the middle of a segment's pulse, and a
coil left driven while the core is stopped overheats, along with its MOSFETs - see SegmentDriver for why a wrong bit
pattern in those registers is worse still. Anything new that halts the core must do the same, or halt only while the
clock sleeps (DISP_ENABLE, PA08, low: nothing flips then).

## Serial commands

While awake, the clock is a USB serial port. It detaches before every sleep, so open its menu first - the menu keeps it awake. The baud rate does not matter (115200 is customary), with one exception: when the port is opened at 1200 baud and closed again, the clock hands over to its bootloader.

A command is one byte. Whatever follows belongs to it until the line has been quiet for a second; then the command is
finished. The answers are lines of the clock's log, each ending in CR LF.

- **`?`** - who are you: answers `Device: EH14` and `Capacity: N`, the number of bytes available for sounds (the
  flash minus its first 4K).
- **`%`** - set the time: six bytes follow, the digits of HHMMSS as numbers 0..9 (not ASCII). The clock sets the
  time once the line is quiet, a second after the last digit (so send the time a second ahead), then shows and
  announces it. An invalid time is ignored.
- **`@`** - a new sound bank: the whole flash is erased, including the settings, and everything that follows is written
  from address `0x1000` on. The log tells how it goes: the flash chip (`Flash manufacturer`, `Flash JEDEC ID`,
  `Flash capacity`), `Erasing chip`, `Chip erased`, `Starting flashing`, `Bytes written: N` every 8K, then
  `Flashing ended`, `Total bytes: N`, and the bank as the clock reads it back: `Data header length: N`, a
  `Sample N: offset O, length L` line for each sound, `Found alarms: N`. Erasing takes from 50ms to a minute,
  depending on the chip; meanwhile the bank waits in the USB buffers.

A sound bank is a big-endian 32-bit header length (4 + 5 bytes per sound), then a 5-byte entry for each sound - its
number and its length in bytes, big-endian - then all the sounds in the order of the entries: 16-bit signed mono,
little-endian, 22050 Hz. The numbers say what a sound is:

|                   |                                                               |
| ----------------- | ------------------------------------------------------------- |
| 0..20, 30, 40, 50 | the numbers                                                   |
| 60, 61            | before and after a time announcement                          |
| 70, 71, 72        | "hodin", "hodina", "hodiny"                                   |
| 80, 81, 82        | "minut", "minuta", "minuty"                                   |
| 90                | the volume test sample (menu A)                               |
| 91, 92            | "time set", "the current time is"                             |
| 93, 94            | "alarm set", "the alarm is"                                   |
| 95, 96            | ding, dong: the silent mode's light coming and going (menu F) |
| 100..             | the alarm sounds, as many as there are                        |

## Hardware notes

- The 64 Mbit flash differs from board to board: AT25SF641, SST26VF064B and similar.
  SpiFlash uses only what they all share - the size from the chip's SFDP table, the write protection cleared either
  way - so any 64 Mbit SPI NOR flash should do.

## Changes from the Arduino firmware

Everything the user sees is as it was, unless listed here. Fixed:

- **Uploads lost every 8601st byte** (a 0xFF in its place, an off-by-one in the 8600-byte buffer): banks are written
  whole now. Clocks keep the old holes until their next upload.
- **L8 and H8 were lost at every power-up** (the settings check refused 8), leaving H set with the silent mode off - a
  clock that never slept. Both are kept; H without a level is no longer possible.
- **Switching the alarm on (menu C) after its time had passed while it was off rang it at once** (the DS3231's stale
  match flag). The flag is cleared first.
- **A press between two plays of the alarm was lost** - it stops the alarm now, as a press during a play always did.
- **The remote's MELODIE never picked the last alarm sound**, and played the same sequence after every power-up.
- **EEEE (no sound bank) never showed properly**: the display was not powered or even set up yet. It is shown now,
  with every segment written.
- **The display's shift registers are cleared first thing after reset**, before the clocks are even set up; they come
  up with anything in them.
- **The test mode's LED blink** drove a pin that was not an output yet.
- **The RTC alarm's flag was cleared over I2C inside the interrupt**, possibly in the middle of a main-loop transfer.
- **The IR receiver** kept shifting bits past the eighth (undefined behavior) and let its "receiving" state, which
  holds off sleep, end at the first bit; it now takes eight bits and stays receiving until the byte is in, or for
  200 ms.
- **A sound stopped with a button left the amplifier on its last sample** (a DC offset into the speaker): every sound
  ends in silence.
- **The serial time command** took anything for digits; a time that is not one is ignored now.
- **The flash is any 64Mbit chip** that answers, and an erase is waited for as long as the chip needs.

New: `params::kFlipDisplayWhileSpeaking` (off, as before) lets the digits follow the clock while it talks.

## AI usage and transparency

All creative decisions, the hardware design and the original firmware are the author's work. AI tools (specifically Claude Opus 5.5) were used to rewrite the code, fix bugs and write documentation. All AI-assisted work was reviewed and tested by the author.

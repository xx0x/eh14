# EH14 firmware (Claude)

Building, flashing, the libraries, the serial commands and the hardware: `README.md`. How the pieces work is
documented in the code itself, in each class's Doxygen notes - read those, and keep them up to date rather than writing
it down anywhere else. `Eh14` is the place to start.

A change is checked by building it with no warnings (`-Wall -Wextra -Wpedantic`), then on the hardware - there are no
unit tests.

## Driving the board over J-Link

With the probe attached, the running firmware can be watched and operated as it is - no debug build. OpenOCD
(`openocd -f interface/jlink.cfg -c "transport select swd" -c "set CHIPNAME samd21" -f target/at91samdXX.cfg`) reads and
writes memory without halting (`read_memory`, `mww`), and so does J-Link Commander (`mem32`, `w4`). Addresses come
from `arm-none-eabi-nm -C build/eh14.elf`, a member's from `arm-none-eabi-gdb -batch -ex "print &eh14::Eh14::app.settings_"
build/eh14.elf`.

- **Sleep.** Asleep without a debugger (true standby, as in use) the SAMD21's bus cannot be reached: OpenOCD hangs on
  the first memory access, and one stuck access froze a clock until it was reset. J-Link Commander's connect copes, and
  leaves the core's debug enable on - after which the SAMD21 keeps its clocks running in standby, so OpenOCD works, and
  the firmware still sleeps as it should (SysTick's interrupt off, `millis` standing still). To test true standby,
  clear it again: `w4 0xE000EDF0 0xA05F0000`.
- **Don't leave it halted.** A halted clock may become unreachable after several seconds. `r` then `g` in J-Link Commander
  brings it back.
- **Buttons:** they pull their pin to ground against the MCU's pull-up, so turning the pull-up into a pull-down (OUT
  bit 0) reads as a press, and the edge wakes the clock. SNOOZE and MENU are PB08 and PB09 (OUTCLR `0x41004494`, OUTSET
  `0x41004498`, masks `0x100` and `0x200`), CHANGE is PA04 (OUTCLR `0x41004414`, OUTSET `0x41004418`, mask `0x10`).
  Write OUTCLR, wait 100ms, write OUTSET. Presses of one button closer together than params::kDebounceMs count as one, and the
  menu needs a second or two between presses for the digits to flip.
- **Sound:** `Voice::debug_sentence_` (`debug_sentence_length_` of them), `debug_frames_` and `debug_play_ms_` are the
  last sentence played, the frames that went to the DMA and how long it took: the frames must add up to the sounds'
  lengths / 2, and the time to the frames at 22061Hz. `AudioOut::debug_underruns_` stays 0. Through a microphone (a
  webcam's, the bench's) the gongs match the bank's sounds sample for sample, the speech hardly at all - the
  microphone's processing - so trust the counters.
- **Sleep, after the fact:** `Sleep::debug_breadcrumb_` and its neighbors live in `.noinit`, which a reset leaves
  alone: where the last sleep got to, and the RTC's count and compare.
- **The flash:** `SpiFlash::debug_sfdp_capacity_` says whether the chip's size came from its SFDP table.
- **Never send `@` to a clock** unless a new sound bank is meant to go in - it erases the whole flash first.

## Structure

- **One class per file pair.** `Foo.hpp` / `Foo.cpp` hold class `Foo` and nothing else, named after it. Types nested
  in it are fine; small header-only helpers (Gpio, Board, IrqPriority) have no `.cpp`.
- **Everything lives in a class.** No free functions and no constants at namespace scope, except for the two files of
  plain constants (below). Helpers that need no object go into a class of static members with a deleted constructor.
- **No anonymous namespaces.** Whatever a `.cpp` needs - constants, helpers, tables, interrupt entry points - is a
  private (static) member of its class, declared in the header.
- **Constants go in the header** as `static constexpr` members, tables included.
- **The hardware objects and the application are static members of `Eh14`** (`Eh14::flash`, `Eh14::app`), and a class
  reaches the others through them, in its `.cpp` - `Init()` takes no references, and no header includes `Eh14.hpp`, so
  there are no circular includes.
- **`hardware/` is the board**, a class per part (each documents its wiring); `usb/` the USB port; `app/` the clock
  itself - Application, its Menu, the Voice, the SoundBank, the Settings, the SerialProtocol, the TestMode.
- **Interrupts only take note of things** (IrqPriority); everything else runs in the main loop. What waits, waits in
  `Eh14::Delay` and its kin, which call `Eh14::RunBackground` meanwhile - USB, the audio buffers, the display.
- **What stays outside a class:** `main`; `extern "C"` functions (IRQ handlers, TinyUSB callbacks), kept to a line
  that hands over to a class; the startup code; `usb_descriptors.c`.
- **Two files of plain constants** - `inline constexpr kName` in a namespace, no class, so whoever tunes the clock has
  one file to work in:
  - `Eh14Params.hpp` (`eh14::params`): what is fixed - the timings, the volume steps, the light thresholds, the
    battery range, params::kFlipDisplayWhileSpeaking.
  - `Eh14Defaults.hpp` (`eh14::defaults`): what the user can change from the menu, as it is before they do.

## Naming

- Classes, structs, methods: `PascalCase`. Namespaces: `eh14`, `eh14::params`, `eh14::defaults`.
- **Accessors are Get / Set / Is:** `GetVolume()`, `SetVolume()`, `IsActive()`; `Has...` where `Is...` reads badly.
  Events taken once are `Take...` (`TakePress()`, `TakeAlarm()`).
- Member variables `snake_case_` with a trailing underscore; locals and parameters `snake_case`.
- **Bench-only variables start with `debug_`:** whatever is there only to work on the board over J-Link
  (`Voice::debug_frames_`, `Sleep::debug_breadcrumb_`). State the firmware runs on keeps its plain name.
- Constants `kPascalCase`, with the unit at the end where there is one: `kSegmentPulseUs`, `kMenuTimeoutMs`.
- `enum class` values `UPPER_SNAKE`; an enum that is counted ends in `COUNT`.
- A class named after the device header's type of the same peripheral clashes with its register macro (the headers
  define `Adc`, `Rtc`, `Port`, ... and `#define ADC ((Adc *)...)`): hence AnalogInputs and RealTimeClock.

## Formatting

- MIT license header on top of every source file, then `#pragma once` in headers.
- Allman braces, 4 spaces, lines up to about 120 columns.
- Includes: a `.cpp` starts with its own header, then `<system>` headers, then project headers as paths from `src/`
  (`"hardware/Display.hpp"`), each group sorted. Include what you use.
- `std::uint32_t` and friends, always with `std::`. No floats: the Cortex-M0+ has no FPU, and nothing here needs them.
- No heap, exceptions or RTTI. The M0+ has no exclusive-access instructions, so what an interrupt shares with the main
  loop is `volatile`, and a read-modify-write of it from the main loop masks interrupts around it.

## Documentation

- Every class gets a Doxygen block: `@class`, `@brief`, `@details` where there is more to say, `@author`, `@date`.
- Comments say why, in plain sentences - what the hardware needs, what would go wrong otherwise. Refer to other members
  with `@ref`.
- **Keep them short.** No filler, history or flourishes ("for years", "to the microsecond"). A value that is dangerous
  to change gets a `@warning`.
- **Don't repeat the code in a comment.** Say what the code cannot: what a thing is for, its unit and range, why it is
  what it is, what goes wrong if it changes. Schematic references, datasheet figures and measurements are welcome next
  to the value they explain.

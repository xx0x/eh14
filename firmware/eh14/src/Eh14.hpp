/*
MIT License

Copyright (c) 2026 Vaclav Mach

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <cstdint>

#include "app/Application.hpp"
#include "hardware/AnalogInputs.hpp"
#include "hardware/AudioOut.hpp"
#include "hardware/Buttons.hpp"
#include "hardware/Display.hpp"
#include "hardware/ExternalInterrupts.hpp"
#include "hardware/I2cBus.hpp"
#include "hardware/IrReceiver.hpp"
#include "hardware/RealTimeClock.hpp"
#include "hardware/Sleep.hpp"
#include "hardware/SpiBus.hpp"
#include "hardware/SpiFlash.hpp"
#include "hardware/StatusLed.hpp"
#include "usb/UsbSerial.hpp"

namespace eh14
{

/**
 * @brief Initializes and owns the EH14's hardware and application.
 * @details
 * The main board (schematics/eh14-main-board-schematics.pdf): an ATSAMD21G18A, a DS3231 RTC, an 8M SPI flash with the
 * sounds, a MAX98357A amplifier, an ATtiny85 IR receiver, three buttons and a Li-ion cell. The display board: four
 * electromechanical digits, their 19V boost converter, the status LED and the light sensor. Each part has a class in
 * hardware/ that documents its wiring; Board has the pins.
 *
 * ### The main loop
 * The application is sequential and waits where the clock waits: for a sound, the digits, a pause. Every wait
 * (@ref Delay, @ref SmartDelay, @ref WaitForDisplay, a sound playing) calls @ref RunBackground: USB, the audio
 * buffers, the next segment flip and the bootloader request. Interrupts only take note of things (IrqPriority);
 * everything else runs in the main loop, so the buses, the flash and TinyUSB need no locks.
 *
 * @warning The display's shift registers power up with random content, and a wrong pattern shorts a half bridge
 *          (SegmentDriver). @ref Init clears them first, before the clocks, with the boost converter off.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Eh14
{
public:
    /**
     * @brief Brings up the display (safely), the clocks, every part, then the application.
     */
    static void Init();

    /**
     * @brief One pass of the main loop.
     */
    static void Loop();

    /**
     * @brief Runs while anything waits: USB, the audio buffers, the display, the application's background work, the
     *        bootloader request. Never blocks, beyond a segment's pulse.
     */
    static void RunBackground();

    /**
     * @brief Waits @p ms, calling @ref RunBackground meanwhile.
     */
    static void Delay(std::uint32_t ms);

    /**
     * @brief Waits @p ms in 10ms steps, calling @ref RunBackground; returns early once a press is waiting.
     */
    static void SmartDelay(std::uint32_t ms);

    /**
     * @brief Waits until the display shows its target and the pause after the last flip is over.
     */
    static void WaitForDisplay();

    /**
     * @brief Hands over to the UF2 bootloader: everything off, the magic word at the top of RAM, reset.
     */
    [[noreturn]] static void EnterBootloader();

    /**
     * @brief Call this when a fatal error occurs. Traps the CPU to halt execution.
     */
    static inline void ErrorHandler()
    {
        __builtin_trap();
    }

    static inline StatusLed status_led;
    static inline Display display;
    static inline ExternalInterrupts eic;
    static inline Buttons buttons;
    static inline IrReceiver ir_receiver;
    static inline I2cBus i2c;
    static inline RealTimeClock rtc;
    static inline SpiBus spi;
    static inline SpiFlash flash;
    static inline AnalogInputs analog_inputs;
    static inline AudioOut audio;
    static inline Sleep sleep;
    static inline UsbSerial usb;
    static inline Application app;

private:
    /**
     * @brief The DS3231's alarm edge: noted for the application; from now on a press only stops the alarm.
     */
    static void HandleRtcAlarmEdge(void *context);

    static inline bool usb_ready_ = false;
    static inline bool app_ready_ = false;
    static inline bool in_background_ = false;
};

} // namespace eh14

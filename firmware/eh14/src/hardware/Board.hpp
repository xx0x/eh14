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

#include "hardware/Gpio.hpp"

namespace eh14
{

/**
 * @class Board
 * @brief The EH14 main board's wiring: which pin of the ATSAMD21G18A (U4) goes where.
 * @details The names follow the nets of schematics/eh14-main-board-schematics.pdf. The display board connects over
 *          the 16-pin ribbon (P1): shift registers, boost converter enable, status LED and light sensor. The IR
 *          receiver is an ATtiny85 (U8) that sends each remote control button as one byte over three wires
 *          (IrReceiver).
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Board
{
public:
    Board() = delete;

    // Buttons, to ground against the MCU's pull-ups, each with its EIC line (EXTINT number).
    static constexpr Pin kSnoozeButton = {1, 8};
    static constexpr std::uint8_t kSnoozeLine = 8;
    static constexpr Pin kMenuButton = {1, 9};
    static constexpr std::uint8_t kMenuLine = 9;
    static constexpr Pin kChangeButton = {0, 4};
    static constexpr std::uint8_t kChangeLine = 4;

    // The DS3231's INT/SQW (U6), open drain with R10 to 3V3: low while an alarm is flagged.
    static constexpr Pin kRtcAlarm = {1, 2};
    static constexpr std::uint8_t kRtcAlarmLine = 2;

    // The ATtiny85's link (TINY_LATCH, TINY_DATA, TINY_CLOCK).
    static constexpr Pin kIrLatch = {0, 6};
    static constexpr std::uint8_t kIrLatchLine = 6;
    static constexpr Pin kIrData = {0, 9};
    static constexpr Pin kIrClock = {0, 15};
    static constexpr std::uint8_t kIrClockLine = 15;

    // The display board: one chain of 8 x 74HC595 (2 per digit), and the boost converter's enable.
    static constexpr Pin kDisplayData = {0, 18};
    static constexpr Pin kDisplayClock = {0, 19};
    static constexpr Pin kDisplayLatch = {0, 16};
    static constexpr Pin kDisplayEnable = {0, 8};
    static constexpr Pin kStatusLed = {0, 17};

    // Analog inputs, with their ADC channels.
    static constexpr Pin kLightSensor = {0, 2};
    static constexpr std::uint8_t kLightSensorChannel = 0;
    static constexpr Pin kBatterySensor = {0, 5};
    static constexpr std::uint8_t kBatterySensorChannel = 5;

    // The MAX98357A amplifier (U5) on I2S serializer 0, and its /SD_MODE (SP_ENABLE).
    static constexpr Pin kI2sSd = {0, 7};
    static constexpr Pin kI2sSck = {0, 10};
    static constexpr Pin kI2sFs = {0, 11};
    static constexpr Pin kSpeakerEnable = {0, 21};

    // The SPI flash (U7) on SERCOM4: MISO on pad 0, MOSI on pad 2, SCK on pad 3 (function D).
    static constexpr Pin kSpiMiso = {0, 12};
    static constexpr Pin kSpiMosi = {1, 10};
    static constexpr Pin kSpiSck = {1, 11};
    static constexpr Pin kFlashCs = {0, 20};

    // The DS3231 on SERCOM3: SDA on pad 0, SCL on pad 1 (function C), with R11/R12 to 3V3.
    static constexpr Pin kI2cSda = {0, 22};
    static constexpr Pin kI2cScl = {0, 23};

    // USB (function G).
    static constexpr Pin kUsbDm = {0, 24};
    static constexpr Pin kUsbDp = {0, 25};
};

} // namespace eh14

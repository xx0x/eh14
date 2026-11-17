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

#include "sam.h"

namespace eh14
{

/**
 * @brief A port pin: its group (0 for PA, 1 for PB) and its number in the group.
 */
struct Pin
{
    std::uint8_t group;
    std::uint8_t number;

    constexpr std::uint32_t Mask() const
    {
        return 1UL << number;
    }
};

/**
 * @class Gpio
 * @brief Port pins, through the PORT registers.
 * @details OUTSET and OUTCLR change only the given pin, so the main loop and interrupts can drive pins without a
 *          lock. With the pull resistor on, OUT selects pull-up or pull-down - also how a button press is simulated
 *          over SWD (CLAUDE.md).
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Gpio
{
public:
    Gpio() = delete;

    enum class Pull
    {
        NONE,
        UP,
        DOWN,
    };

    /**
     * @brief The pin multiplexer's peripheral functions, lettered as in the datasheet.
     */
    enum class Function : std::uint8_t
    {
        A = 0, ///< EIC
        B = 1, ///< Analog: ADC
        C = 2, ///< SERCOM
        D = 3, ///< SERCOM, the alternative pads
        E = 4,
        F = 5,
        G = 6, ///< I2S, USB
        H = 7,
    };

    /**
     * @brief Makes @p pin an output at @p high. The level is set before the driver turns on, so there is no glitch.
     */
    static void SetOutput(Pin pin, bool high)
    {
        Write(pin, high);
        PORT->Group[pin.group].PINCFG[pin.number].reg = PORT_PINCFG_INEN;
        PORT->Group[pin.group].DIRSET.reg = pin.Mask();
    }

    static void SetInput(Pin pin, Pull pull)
    {
        PORT->Group[pin.group].DIRCLR.reg = pin.Mask();
        if (pull == Pull::UP)
        {
            PORT->Group[pin.group].OUTSET.reg = pin.Mask();
        }
        else if (pull == Pull::DOWN)
        {
            PORT->Group[pin.group].OUTCLR.reg = pin.Mask();
        }
        PORT->Group[pin.group].PINCFG[pin.number].reg =
            PORT_PINCFG_INEN | ((pull != Pull::NONE) ? PORT_PINCFG_PULLEN : 0);
    }

    /**
     * @brief Hands @p pin to a peripheral. Keeps the input buffer and pull resistor as set (an EIC input with a
     *        pull-up needs them).
     */
    static void SetFunction(Pin pin, Function function)
    {
        const std::uint8_t index = pin.number / 2U;
        const std::uint8_t value = static_cast<std::uint8_t>(function);
        if ((pin.number & 1U) != 0U)
        {
            PORT->Group[pin.group].PMUX[index].bit.PMUXO = value;
        }
        else
        {
            PORT->Group[pin.group].PMUX[index].bit.PMUXE = value;
        }
        PORT->Group[pin.group].PINCFG[pin.number].bit.PMUXEN = 1;
    }

    static void Write(Pin pin, bool high)
    {
        if (high)
        {
            PORT->Group[pin.group].OUTSET.reg = pin.Mask();
        }
        else
        {
            PORT->Group[pin.group].OUTCLR.reg = pin.Mask();
        }
    }

    static bool Read(Pin pin)
    {
        return (PORT->Group[pin.group].IN.reg & pin.Mask()) != 0U;
    }
};

} // namespace eh14

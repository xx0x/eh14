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

namespace eh14
{

/**
 * @class StatusLed
 * @brief The status LED under the digits (LED1 on the display board).
 * @details It sits next to the light sensor, so the menu inhibits it while the silent mode's sensitivity is set (menu
 *          F); @ref Set then does nothing. Driven from the main loop and the IR receiver's interrupt: a single
 *          register write.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class StatusLed
{
public:
    bool Init();

    void Set(bool on);

    void SetInhibited(bool inhibited)
    {
        inhibited_ = inhibited;
    }

    /**
     * @brief Flashes @p count times, @p on_ms on and @p off_ms off, waiting through it (Eh14::Delay).
     */
    void Flash(unsigned count, unsigned on_ms, unsigned off_ms);

private:
    volatile bool inhibited_ = false;
};

} // namespace eh14

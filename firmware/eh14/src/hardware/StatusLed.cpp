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

#include "hardware/StatusLed.hpp"

#include "Eh14.hpp"
#include "hardware/Board.hpp"
#include "hardware/Gpio.hpp"

namespace eh14
{

bool StatusLed::Init()
{
    Gpio::SetOutput(Board::kStatusLed, false);
    return true;
}

void StatusLed::Set(bool on)
{
    if (!inhibited_)
    {
        Gpio::Write(Board::kStatusLed, on);
    }
}

void StatusLed::Flash(unsigned count, unsigned on_ms, unsigned off_ms)
{
    for (unsigned i = 0; i < count; i++)
    {
        Set(true);
        Eh14::Delay(on_ms);
        Set(false);
        Eh14::Delay(off_ms);
    }
}

} // namespace eh14

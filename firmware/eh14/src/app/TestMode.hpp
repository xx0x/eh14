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
 * @class TestMode
 * @brief The assembly check: MENU and CHANGE held at power-on.
 * @details
 * While both are held, the status LED blinks (Application::WaitForTestModeButtons); on release, the clock runs this
 * sequence, then starts normally:
 * 1. SELF TEST, then the digits 0..9 in every position, flipping each segment both ways.
 * 2. The number of alarm sounds, and the last one played: the whole bank is there.
 * 3. The alarm: the time set to 06:59:58, the alarm to 07:00. After 3s "A OK", or "AbAd" if the DS3231 never pulled
 *    its alarm line.
 * 4. The light sensor's reading, then again with the status LED on - the LED must not affect the sensor.
 *
 * @warning This leaves the time at 07:00 and the alarm on: set both afterwards.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class TestMode
{
public:
    void Run();

private:
};

} // namespace eh14

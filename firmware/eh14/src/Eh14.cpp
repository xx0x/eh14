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

#include "Eh14.hpp"

#include "Eh14Params.hpp"
#include "hardware/Board.hpp"
#include "hardware/Clocks.hpp"
#include "hardware/SegmentDriver.hpp"
#include "hardware/Timebase.hpp"
#include "sam.h"

namespace eh14
{

void Eh14::Init()
{
    // First: the boost converter off and the display's shift registers cleared (random at power-up; a wrong pattern
    // shorts a half bridge). Runs at the bootloader's clock.
    SegmentDriver::Init();

    Clocks::Init();
    Timebase::Init();

    // MENU and CHANGE held at startup select the test mode.
    if (!(status_led.Init() && display.Init() && eic.Init() && buttons.Init() && ir_receiver.Init()))
    {
        ErrorHandler();
    }
    const bool test_mode = Application::WaitForTestModeButtons();

    if (!usb.Init())
    {
        ErrorHandler();
    }
    usb_ready_ = true;
    usb.Println("EH14 startup\n");

    // If the DS3231 does not answer, the clock runs without the time.
    i2c.Init();
    rtc.Init();
    eic.Attach(Board::kRtcAlarmLine, ExternalInterrupts::Edge::FALLING, true, HandleRtcAlarmEdge, nullptr);
    eic.Start();

    if (!(sleep.Init() && spi.Init() && analog_inputs.Init() && audio.Init()))
    {
        ErrorHandler();
    }
    // A flash that does not answer reads as an empty bank, shown as EEEE.
    flash.Init();

    if (!app.Init(test_mode))
    {
        ErrorHandler();
    }
    app_ready_ = true;
}

void Eh14::Loop()
{
    RunBackground();
    app.Loop();
}

void Eh14::RunBackground()
{
    if (in_background_)
    {
        return;
    }
    in_background_ = true;

    if (usb_ready_)
    {
        usb.Task();
    }
    audio.Refill();
    display.Update();
    if (app_ready_)
    {
        app.RunBackground();
    }

    in_background_ = false;

    if (usb_ready_ && usb.IsBootloaderRequested())
    {
        EnterBootloader();
    }
}

void Eh14::Delay(std::uint32_t ms)
{
    const std::uint32_t start = Timebase::GetMillis();
    while ((Timebase::GetMillis() - start) < ms)
    {
        RunBackground();
    }
}

void Eh14::SmartDelay(std::uint32_t ms)
{
    for (std::uint32_t step = 0; step < ms / 10U; step++)
    {
        Delay(10);
        if (buttons.IsAnyPressedIncludingSound())
        {
            return;
        }
    }
}

void Eh14::WaitForDisplay()
{
    while (!display.IsIdle())
    {
        RunBackground();
    }
}

void Eh14::EnterBootloader()
{
    // RunBackground calls this between segment flips, never during one.
    audio.Stop();
    audio.SetAmplifier(false);
    display.SetPower(false);
    SegmentDriver::Clear();

    __disable_irq();
    *reinterpret_cast<volatile std::uint32_t *>(UsbSerial::kBootloaderMagicAddress) = UsbSerial::kBootloaderMagic;
    NVIC_SystemReset();
}

void Eh14::HandleRtcAlarmEdge(void * /*context*/)
{
    rtc.MarkAlarm();
    buttons.SetAlarmRinging(true);
}

} // namespace eh14

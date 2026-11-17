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

#include "app/SerialProtocol.hpp"

#include <span>

#include "Eh14.hpp"
#include "app/Application.hpp"
#include "app/SoundBank.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

void SerialProtocol::Poll()
{
    UsbSerial &usb = Eh14::usb;
    if (usb.Available() == 0U)
    {
        return;
    }

    mode_ = Mode::NONE;
    std::uint32_t last_received_ms = Timebase::GetMillis();
    while (true)
    {
        std::array<std::uint8_t, 64> bytes = {};
        const std::uint32_t count = usb.Read(bytes);
        if (count > 0U)
        {
            last_received_ms = Timebase::GetMillis();
            for (std::uint32_t i = 0; i < count; i++)
            {
                HandleByte(bytes[i]);
            }
            continue;
        }
        if ((Timebase::GetMillis() - last_received_ms) > kSessionTimeoutMs)
        {
            break;
        }
        Eh14::RunBackground();
    }

    switch (mode_)
    {
    case Mode::UPLOAD:
        EndUpload();
        break;
    case Mode::SET_TIME:
        SetTime();
        break;
    default:
        break;
    }
    mode_ = Mode::NONE;
}

void SerialProtocol::HandleByte(std::uint8_t byte)
{
    switch (mode_)
    {
    case Mode::UPLOAD:
        UploadByte(byte);
        return;
    case Mode::SET_TIME:
        if (time_count_ < kTimeDigits)
        {
            time_digits_[time_count_++] = byte;
        }
        return;
    default:
        break;
    }

    if (byte == kCommandUpload)
    {
        mode_ = Mode::UPLOAD;
        BeginUpload();
    }
    else if (byte == kCommandDevice)
    {
        Eh14::usb.Println("Device: EH14");
        Eh14::usb.Print("Capacity: ");
        const std::uint32_t capacity = Eh14::flash.GetCapacity();
        Eh14::usb.Println((capacity > SoundBank::kHeaderAddress) ? capacity - SoundBank::kHeaderAddress : 0U);
    }
    else if (byte == kCommandSetTime)
    {
        mode_ = Mode::SET_TIME;
        time_count_ = 0;
    }
}

void SerialProtocol::BeginUpload()
{
    UsbSerial &log = Eh14::usb;
    Eh14::Delay(20);
    Eh14::display.Show({Glyph::F, Glyph::L, Glyph::A, Display::Digit(5)});
    Eh14::WaitForDisplay();
    Eh14::Delay(20);

    address_ = SoundBank::kHeaderAddress;
    page_fill_ = 0;
    next_report_ = address_ + kReportEveryBytes;

    SoundBank::LogFlash();
    log.Println("");
    log.Println("Erasing chip");
    // Meanwhile the bank waits in the USB buffers; flow control holds the host off.
    if (Eh14::flash.StartChipErase())
    {
        const std::uint32_t start = Timebase::GetMillis();
        while (Eh14::flash.IsBusy() && (Timebase::GetMillis() - start) < kChipEraseTimeoutMs)
        {
            Eh14::RunBackground();
        }
    }
    log.Println("Chip erased");
    log.Println("Starting flashing");
}

void SerialProtocol::UploadByte(std::uint8_t byte)
{
    if (address_ + page_fill_ >= Eh14::flash.GetCapacity())
    {
        return;
    }
    page_[page_fill_++] = byte;
    if (page_fill_ < page_.size())
    {
        return;
    }

    Eh14::flash.Program(address_, page_);
    address_ += static_cast<std::uint32_t>(page_.size());
    page_fill_ = 0;
    if (address_ >= next_report_)
    {
        next_report_ += kReportEveryBytes;
        Eh14::usb.Print("Bytes written: ");
        Eh14::usb.Println(address_);
        led_on_ = !led_on_;
        Eh14::status_led.Set(led_on_);
    }
}

void SerialProtocol::EndUpload()
{
    UsbSerial &log = Eh14::usb;
    log.Println("Flashing ended");
    if (address_ + page_fill_ <= SoundBank::kHeaderAddress)
    {
        return;
    }
    if (page_fill_ > 0U)
    {
        Eh14::flash.Program(address_, std::span<const std::uint8_t>(page_.data(), page_fill_));
        address_ += static_cast<std::uint32_t>(page_fill_);
        page_fill_ = 0;
    }
    log.Print("Total bytes: ");
    log.Println(address_);

    Eh14::app.ReloadSoundBank();
    led_on_ = false;
    Eh14::status_led.Set(false);
    Eh14::display.Clear();
    Eh14::WaitForDisplay();
}

void SerialProtocol::SetTime()
{
    if (time_count_ < kTimeDigits)
    {
        return;
    }
    for (const std::uint8_t digit : time_digits_)
    {
        if (digit > 9U)
        {
            return;
        }
    }
    const RealTimeClock::Time time = {static_cast<std::uint8_t>((time_digits_[0] * 10U) + time_digits_[1]),
                            static_cast<std::uint8_t>((time_digits_[2] * 10U) + time_digits_[3]),
                            static_cast<std::uint8_t>((time_digits_[4] * 10U) + time_digits_[5])};
    if (time.hour > 23U || time.minute > 59U || time.second > 59U)
    {
        return;
    }

    Eh14::rtc.SetTime(time);
    const RealTimeClock::Time now = Eh14::rtc.GetTime();
    Eh14::display.ShowTime(now.hour, now.minute);
    Eh14::WaitForDisplay();
    Eh14::app.AnnounceTimeSet(SoundBank::kTimeSet, SoundBank::kTimeCurrent, now);
}

} // namespace eh14

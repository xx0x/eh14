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

#include "app/SoundBank.hpp"

#include "Eh14.hpp"

namespace eh14
{

bool SoundBank::Load()
{
    SpiFlash &flash = Eh14::flash;
    UsbSerial &log = Eh14::usb;
    LogFlash();

    const std::uint32_t header_length = flash.ReadUint32BigEndian(kHeaderAddress);
    log.Print("Data header length: ");
    log.Println(header_length);
    if (header_length > kMaxHeaderLength)
    {
        log.Println("Header too long, ignoring...");
        return false;
    }

    sounds_ = {};
    alarm_count_ = 0;
    std::uint32_t offset = kHeaderAddress + header_length;
    for (std::uint32_t entry = kHeaderAddress + 4U; entry < kHeaderAddress + header_length; entry += 5U)
    {
        const std::uint8_t id = flash.ReadByte(entry);
        if (id >= kMaxSounds)
        {
            break;
        }
        const std::uint32_t length = flash.ReadUint32BigEndian(entry + 1U);
        sounds_[id] = {offset, length};
        offset += length;
        if (id >= kFirstAlarm)
        {
            alarm_count_++;
        }
    }

    for (std::uint8_t id = 0; id < kMaxSounds; id++)
    {
        if (sounds_[id].length == 0U)
        {
            continue;
        }
        log.Print("Sample ");
        if (id < 10U)
        {
            log.Print(" ");
        }
        log.Print(id);
        log.Print(": offset ");
        log.Print(sounds_[id].offset);
        log.Print(", length ");
        log.Println(sounds_[id].length);
    }
    log.Print("Found alarms: ");
    log.Println(alarm_count_);
    return true;
}

void SoundBank::LogFlash()
{
    SpiFlash &flash = Eh14::flash;
    UsbSerial &log = Eh14::usb;
    log.Print("Flash manufacturer: ");
    log.Println(flash.GetManufacturerId());
    log.Print("Flash JEDEC ID: ");
    log.Println(flash.GetJedecId());
    log.Print("Flash capacity: ");
    log.Println(flash.GetCapacity());
}

} // namespace eh14

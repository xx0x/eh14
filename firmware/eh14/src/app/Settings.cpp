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

#include "app/Settings.hpp"

#include "Eh14.hpp"
#include "Eh14Defaults.hpp"
#include "Eh14Params.hpp"

namespace eh14
{

bool Settings::Load(std::uint8_t alarm_count)
{
    volume_ = defaults::kVolume;
    alarm_sound_ = defaults::kAlarmSound;
    silent_level_ = defaults::kSilentLevel;
    silent_high_power_ = defaults::kSilentHighPower;
    alarm_loops_ = defaults::kAlarmLoops;
    intro_enabled_ = false;

    Bytes bytes = {};
    Eh14::flash.Read(kAddress, bytes);

    if (bytes[Index(Field::VOLUME)] < params::kVolumes.size())
    {
        volume_ = bytes[Index(Field::VOLUME)];
    }
    if (bytes[Index(Field::ALARM_SOUND)] < alarm_count)
    {
        alarm_sound_ = bytes[Index(Field::ALARM_SOUND)];
    }
    // 8 is a level too: L8 and H8.
    if (bytes[Index(Field::SILENT_LEVEL)] <= params::kSilentThresholds.size())
    {
        silent_level_ = bytes[Index(Field::SILENT_LEVEL)];
    }
    // H without a level would keep the clock awake with the silent mode off.
    silent_high_power_ = bytes[Index(Field::SILENT_HIGH_POWER)] == 1U && silent_level_ > 0U;
    if (bytes[Index(Field::ALARM_LOOPS)] < params::kAlarmLoops.size())
    {
        alarm_loops_ = bytes[Index(Field::ALARM_LOOPS)];
    }
    intro_enabled_ = bytes[Index(Field::INTRO_ENABLED)] == 1U;

    Eh14::usb.Println("Reading settings from flash: ");
    LogBytes(bytes);
    return true;
}

void Settings::Save()
{
    SpiFlash &flash = Eh14::flash;
    UsbSerial &log = Eh14::usb;
    Bytes stored = {};
    flash.Read(kAddress, stored);
    const Bytes bytes = ToBytes();
    if (stored == bytes)
    {
        log.Println("Settings same, not saving to flash.");
        return;
    }

    log.Print("Flash erase sector 0 ");
    log.Println(flash.EraseSector(kAddress) ? " ... OK" : " ... not OK");

    log.Println("Saving settings to flash: ");
    LogBytes(bytes);
    log.Println(flash.Program(kAddress, bytes) ? " ... OK" : " ... not OK");
}

Settings::Bytes Settings::ToBytes() const
{
    Bytes bytes = {};
    bytes[Index(Field::VOLUME)] = volume_;
    bytes[Index(Field::ALARM_SOUND)] = alarm_sound_;
    bytes[Index(Field::SILENT_LEVEL)] = silent_level_;
    bytes[Index(Field::SILENT_HIGH_POWER)] = silent_high_power_ ? 1U : 0U;
    bytes[Index(Field::ALARM_LOOPS)] = alarm_loops_;
    bytes[Index(Field::INTRO_ENABLED)] = intro_enabled_ ? 1U : 0U;
    return bytes;
}

void Settings::LogBytes(const Bytes &bytes)
{
    for (const std::uint8_t byte : bytes)
    {
        Eh14::usb.PrintHex(byte);
        Eh14::usb.Print(" ");
    }
}

} // namespace eh14

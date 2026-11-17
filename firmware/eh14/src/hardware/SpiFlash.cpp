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

#include "hardware/SpiFlash.hpp"

#include <array>

#include "Eh14.hpp"
#include "hardware/Board.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

bool SpiFlash::Init()
{
    Gpio::SetOutput(Board::kFlashCs, true);

    std::array<std::uint8_t, 3> id = {};
    Select();
    Eh14::spi.Transfer(kCmdJedecId);
    Eh14::spi.Read(id);
    Deselect();
    jedec_id_ = (static_cast<std::uint32_t>(id[0]) << 16) | (static_cast<std::uint32_t>(id[1]) << 8) | id[2];
    global_unlock_ = id[0] == kSstManufacturer && id[1] == kSst26Type;
    unprotected_ = false;

    std::array<std::uint8_t, 2> manufacturer = {};
    SendAddressCommand(kCmdManufacturerId, 0);
    Eh14::spi.Read(manufacturer);
    Deselect();
    manufacturer_id_ = static_cast<std::uint16_t>((manufacturer[0] << 8) | manufacturer[1]);

    // No chip reads as all zeros or all ones.
    const bool present = (id[0] != 0x00U && id[0] != 0xFFU);
    if (!present)
    {
        capacity_ = 0;
        return false;
    }
    capacity_ = ReadSfdpCapacity();
    debug_sfdp_capacity_ = capacity_;
    if (capacity_ == 0U)
    {
        capacity_ = CapacityFromJedec(id[0], id[1], id[2]);
    }
    if (capacity_ == 0U)
    {
        capacity_ = kDefaultCapacity;
    }
    return true;
}

std::uint32_t SpiFlash::ReadSfdpCapacity()
{
    // SFDP header, then the first parameter header (always the JEDEC basic flash parameter table), its pointer in
    // bytes 12..14.
    std::array<std::uint8_t, 16> header = {};
    SendAddressCommand(kCmdReadSfdp, 0);
    Eh14::spi.Transfer(0);
    Eh14::spi.Read(header);
    Deselect();
    const std::uint32_t signature = header[0] | (header[1] << 8) | (header[2] << 16) |
                                    (static_cast<std::uint32_t>(header[3]) << 24);
    if (signature != kSfdpSignature || header[8] != 0x00U)
    {
        return 0;
    }
    const std::uint32_t table = header[12] | (header[13] << 8) | (header[14] << 16);

    // The table's second word is the density: bits - 1, or with bit 31 set, log2 of the bits.
    std::array<std::uint8_t, 4> word = {};
    SendAddressCommand(kCmdReadSfdp, table + 4U);
    Eh14::spi.Transfer(0);
    Eh14::spi.Read(word);
    Deselect();
    const std::uint32_t density = word[0] | (word[1] << 8) | (word[2] << 16) | (static_cast<std::uint32_t>(word[3]) << 24);

    std::uint64_t bits = 0;
    if ((density & 0x80000000UL) != 0U)
    {
        const std::uint32_t exponent = density & 0x7FFFFFFFUL;
        bits = (exponent < 40U) ? (1ULL << exponent) : 0U;
    }
    else
    {
        bits = static_cast<std::uint64_t>(density) + 1U;
    }
    const std::uint64_t bytes = bits / 8U;
    return (bytes >= kMinCapacity && bytes <= kMaxCapacity) ? static_cast<std::uint32_t>(bytes) : 0U;
}

std::uint32_t SpiFlash::CapacityFromJedec(std::uint8_t manufacturer, std::uint8_t type, std::uint8_t code)
{
    if (manufacturer == kSstManufacturer)
    {
        // SST's own size codes: SST26VF016B/032B/064B (41h..43h), SST25VF064C (4Bh).
        if (type == kSst26Type && code >= 0x41U && code <= 0x43U)
        {
            return 2UL << (20U + (code - 0x41U));
        }
        if (type == kSst25Type && code == 0x4BU)
        {
            return 8UL << 20;
        }
        return 0;
    }
    if (manufacturer == kSpansionManufacturer && type == 0x02U && code == 0x16U)
    {
        // The S25FL064P codes 64Mbit one step too small.
        return 8UL << 20;
    }
    // Others: 2^code bytes.
    if (code >= 0x14U && code <= 0x1AU)
    {
        return 1UL << code;
    }
    return 0;
}

void SpiFlash::Read(std::uint32_t address, std::span<std::uint8_t> data)
{
    SendAddressCommand(kCmdRead, address);
    Eh14::spi.Read(data);
    Deselect();
}

std::uint32_t SpiFlash::ReadUint32BigEndian(std::uint32_t address)
{
    std::array<std::uint8_t, 4> bytes = {};
    Read(address, bytes);
    return (static_cast<std::uint32_t>(bytes[0]) << 24) | (static_cast<std::uint32_t>(bytes[1]) << 16) |
           (static_cast<std::uint32_t>(bytes[2]) << 8) | bytes[3];
}

std::uint8_t SpiFlash::ReadByte(std::uint32_t address)
{
    std::uint8_t byte = 0;
    Read(address, std::span<std::uint8_t>(&byte, 1));
    return byte;
}

bool SpiFlash::EraseSector(std::uint32_t address)
{
    if (!PrepareWrite())
    {
        return false;
    }
    SendAddressCommand(kCmdSectorErase, address);
    Deselect();
    return WaitReady(kSectorEraseTimeoutMs);
}

bool SpiFlash::StartChipErase()
{
    if (!PrepareWrite())
    {
        return false;
    }
    SendCommand(kCmdChipErase);
    return true;
}

bool SpiFlash::IsBusy()
{
    Select();
    Eh14::spi.Transfer(kCmdReadStatus);
    const std::uint8_t status = Eh14::spi.Transfer(0);
    Deselect();
    return (status & kStatusBusy) != 0U;
}

bool SpiFlash::Program(std::uint32_t address, std::span<const std::uint8_t> data)
{
    while (!data.empty())
    {
        // A page program wraps within its page, so split at page boundaries.
        const std::uint32_t room = kPageSize - (address % kPageSize);
        const std::size_t length = (data.size() < room) ? data.size() : room;
        if (!PrepareWrite())
        {
            return false;
        }
        SendAddressCommand(kCmdPageProgram, address);
        Eh14::spi.Write(data.first(length));
        Deselect();
        if (!WaitReady(kProgramTimeoutMs))
        {
            return false;
        }
        address += length;
        data = data.subspan(length);
    }
    return true;
}

bool SpiFlash::PrepareWrite()
{
    if (!WaitReady(kSectorEraseTimeoutMs))
    {
        return false;
    }
    if (global_unlock_)
    {
        SendCommand(kCmdWriteEnable);
        SendCommand(kCmdGlobalUnlock);
    }
    else if (!unprotected_)
    {
        Unprotect();
    }
    SendCommand(kCmdWriteEnable);
    return true;
}

bool SpiFlash::Unprotect()
{
    Select();
    Eh14::spi.Transfer(kCmdReadStatus);
    const std::uint8_t status = Eh14::spi.Transfer(0);
    Deselect();
    if ((status & kStatusProtection) != 0U)
    {
        SendCommand(kCmdWriteEnable);
        Select();
        Eh14::spi.Transfer(kCmdWriteStatus);
        Eh14::spi.Transfer(0);
        Deselect();
        if (!WaitReady(kSectorEraseTimeoutMs))
        {
            return false;
        }
    }
    unprotected_ = true;
    return true;
}

bool SpiFlash::WaitReady(std::uint32_t timeout_ms)
{
    const std::uint32_t start = Timebase::GetMillis();
    while (IsBusy())
    {
        if ((Timebase::GetMillis() - start) > timeout_ms)
        {
            return false;
        }
    }
    return true;
}

void SpiFlash::SendAddressCommand(std::uint8_t command, std::uint32_t address)
{
    Select();
    const std::array<std::uint8_t, 4> bytes = {command, static_cast<std::uint8_t>(address >> 16),
                                               static_cast<std::uint8_t>(address >> 8),
                                               static_cast<std::uint8_t>(address)};
    Eh14::spi.Write(bytes);
}

void SpiFlash::SendCommand(std::uint8_t command)
{
    Select();
    Eh14::spi.Transfer(command);
    Deselect();
}

void SpiFlash::Select()
{
    Gpio::Write(Board::kFlashCs, false);
}

void SpiFlash::Deselect()
{
    Gpio::Write(Board::kFlashCs, true);
}

} // namespace eh14

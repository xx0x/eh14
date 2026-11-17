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
#include <span>

namespace eh14
{

/**
 * @class SpiFlash
 * @brief The 64Mbit SPI flash (U7): the sounds, and the settings in its first sector.
 * @details
 * Boards carry different 64Mbit chips (an SST26VF064B per the schematic, an Adesto AT25SF641 on the bench, possibly
 * others), so only commands common to SPI NOR flash are used: read (03h), page program (02h, 256 bytes), 4K sector
 * erase (20h), chip erase (C7h), read status (05h), write enable (06h). Plain SPI mode; WP# and HOLD# are tied high.
 *
 * Write protection at power-up differs:
 * - SST26: every block protected; cleared by the global unlock (ULBPR, 98h).
 * - Others: block protection bits in the status register, set at power-up on some (SST25, AT25DF). They are cleared
 *   once, before the first write, and only if set (the status register is flash too).
 *
 * Protection is never set again.
 *
 * The size comes from the SFDP table (JESD216), else from the JEDEC ID, else the board's 8M default. A chip that
 * answers never reports 0 bytes (the device command reports this size, SerialProtocol).
 *
 * Erase and program busy-wait on the status register: a page takes a few ms, a sector up to a few hundred. A chip
 * erase takes 50ms on the SST26 but tens of seconds on most others, so @ref StartChipErase returns at once and
 * @ref IsBusy tells when it is done.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class SpiFlash
{
public:
    static constexpr std::uint32_t kSectorSize = 4096;
    static constexpr std::uint32_t kPageSize = 256;

    /**
     * @brief Reads the chip's IDs and size.
     * @return False if no chip answers.
     */
    bool Init();

    void Read(std::uint32_t address, std::span<std::uint8_t> data);

    std::uint32_t ReadUint32BigEndian(std::uint32_t address);

    std::uint8_t ReadByte(std::uint32_t address);

    /**
     * @brief Erases the 4K sector at @p address.
     */
    bool EraseSector(std::uint32_t address);

    /**
     * @brief Starts erasing the whole chip without waiting; @ref IsBusy tells when it is done.
     */
    bool StartChipErase();

    bool IsBusy();

    /**
     * @brief Programs @p data from @p address, page by page. The range must be erased.
     */
    bool Program(std::uint32_t address, std::span<const std::uint8_t> data);

    /**
     * @brief The size in bytes (from SFDP, the JEDEC ID or the default); 0 if no chip answered.
     */
    std::uint32_t GetCapacity() const
    {
        return capacity_;
    }

    /**
     * @brief Manufacturer and device ID from command 90h, shown in the log as the manufacturer.
     */
    std::uint16_t GetManufacturerId() const
    {
        return manufacturer_id_;
    }

    /**
     * @brief Manufacturer, memory type and capacity code: 0xBF2643 for the SST26VF064B.
     */
    std::uint32_t GetJedecId() const
    {
        return jedec_id_;
    }

    /**
     * @brief The size from the SFDP table, 0 if there was none (bench diagnostics).
     */
    static inline volatile std::uint32_t debug_sfdp_capacity_ = 0;

private:
    static constexpr std::uint8_t kCmdRead = 0x03;
    static constexpr std::uint8_t kCmdPageProgram = 0x02;
    static constexpr std::uint8_t kCmdSectorErase = 0x20;
    static constexpr std::uint8_t kCmdChipErase = 0xC7;
    static constexpr std::uint8_t kCmdWriteEnable = 0x06;
    static constexpr std::uint8_t kCmdReadStatus = 0x05;
    static constexpr std::uint8_t kCmdJedecId = 0x9F;
    static constexpr std::uint8_t kCmdGlobalUnlock = 0x98;
    static constexpr std::uint8_t kCmdManufacturerId = 0x90;
    static constexpr std::uint8_t kCmdWriteStatus = 0x01;
    static constexpr std::uint8_t kCmdReadSfdp = 0x5A;

    static constexpr std::uint8_t kStatusBusy = 0x01;

    /**
     * @brief Status register protection bits on any chip: BP0..BP3 (bits 2..5), TB or BP4 (6), SRP (7). All clear
     *        means unprotected on every chip.
     */
    static constexpr std::uint8_t kStatusProtection = 0xFC;

    static constexpr std::uint8_t kSstManufacturer = 0xBF;
    static constexpr std::uint8_t kSst26Type = 0x26;
    static constexpr std::uint8_t kSst25Type = 0x25;
    static constexpr std::uint8_t kSpansionManufacturer = 0x01;

    /**
     * @brief The board's design size: 64Mbit.
     */
    static constexpr std::uint32_t kDefaultCapacity = 8UL << 20;

    static constexpr std::uint32_t kMinCapacity = 1UL << 20;
    static constexpr std::uint32_t kMaxCapacity = 64UL << 20;

    /**
     * @brief "SFDP" as a little-endian word.
     */
    static constexpr std::uint32_t kSfdpSignature = 0x50444653;

    // Timeouts: the slower of the known chips, with margin.
    static constexpr std::uint32_t kSectorEraseTimeoutMs = 500;
    static constexpr std::uint32_t kProgramTimeoutMs = 20;

    void Select();
    void Deselect();
    void SendAddressCommand(std::uint8_t command, std::uint32_t address);
    void SendCommand(std::uint8_t command);
    bool PrepareWrite();
    bool WaitReady(std::uint32_t timeout_ms);
    bool Unprotect();

    /**
     * @brief The size from the chip's SFDP table, 0 if it has none.
     */
    std::uint32_t ReadSfdpCapacity();

    /**
     * @brief The size from the JEDEC capacity code, 0 if unknown.
     */
    static std::uint32_t CapacityFromJedec(std::uint8_t manufacturer, std::uint8_t type, std::uint8_t code);

    std::uint32_t jedec_id_ = 0;
    std::uint32_t capacity_ = 0;
    std::uint16_t manufacturer_id_ = 0;
    bool global_unlock_ = false;
    bool unprotected_ = false;
};

} // namespace eh14

#!/usr/bin/env python3
# MIT License
#
# Copyright (c) 2026 Vaclav Mach
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Updates the EH14's firmware over USB, without a probe.

The clock has to be awake - open its menu first. The script opens the clock's serial port at 1200 baud and closes
it again, which makes the firmware hand over to the UF2 bootloader (the Arduino IDE's way). The bootloader
shows up as the EH14_FWUPDT drive (FEATHERBOOT on clocks with the older bootloader), and the .uf2 file copied onto it
is the new firmware; the clock restarts with it by itself.

Usage: flash_usb.py eh14.uf2 [--port /dev/cu.usbmodemXXXX]
With no port given, the one serial port that looks like a USB modem is used - with pyserial installed, the one with
the EH14's VID and PID (eh14_serial.py), so a clock with older firmware is found too.
"""

import argparse
import os
import shutil
import sys
import termios
import time

from eh14_serial import find_port

# The bootloader's drive: the EH14's own name first, then the one of the bootloader the clocks were made with.
DRIVE_NAMES = ["EH14_FWUPDT", "FEATHERBOOT"]
DRIVE_TIMEOUT_S = 20


def touch_1200(port):
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[4] = attrs[5] = termios.B1200
    attrs[2] |= termios.HUPCL  # DTR drops when the port closes
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    time.sleep(0.1)
    os.close(fd)


def drive_candidates():
    user = os.environ.get("USER", "")
    return [path for name in DRIVE_NAMES for path in (f"/Volumes/{name}", f"/media/{user}/{name}",
                                                      f"/run/media/{user}/{name}")]


def wait_for_drive():
    deadline = time.time() + DRIVE_TIMEOUT_S
    while time.time() < deadline:
        for path in drive_candidates():
            if os.path.isfile(os.path.join(path, "INFO_UF2.TXT")):
                return path
        time.sleep(0.25)
    sys.exit(f"The bootloader's drive ({' or '.join(DRIVE_NAMES)}) did not show up.")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("uf2")
    parser.add_argument("--port")
    args = parser.parse_args()

    drive = next((p for p in drive_candidates() if os.path.isfile(os.path.join(p, "INFO_UF2.TXT"))), None)
    if drive is None:
        port = args.port or find_port()
        print(f"Asking {port} for the bootloader...")
        touch_1200(port)
        drive = wait_for_drive()
    print(f"Copying {os.path.basename(args.uf2)} onto {drive}...")
    try:
        shutil.copyfile(args.uf2, os.path.join(drive, "NEW.UF2"))
    except OSError:
        # The bootloader resets the moment the last block is in, which can cut the copy's ending short.
        pass
    print("Done - the clock restarts with the new firmware.")


if __name__ == "__main__":
    main()

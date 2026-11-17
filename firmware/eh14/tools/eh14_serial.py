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

"""Finding the EH14's serial port, for the tools here."""

import glob
import os
import re
import subprocess
import sys

# The EH14's VID and PID, newest first: the clock's own, then Arduino's, which the firmware before 2026 used.
EH14_IDS = [(0xCAFE, 0xE015), (0x2341, 0x804D)]
# The EH14 bootloader's own serial port (bossac's), which is never the clock.
BOOTLOADER_IDS = [(0xCAFE, 0xE016)]


def usb_serial_ports():
    """Every USB serial port with its VID and PID, as (vid, pid, device) - from pyserial when it is installed, else
    from the system itself: ioreg on macOS, sysfs on Linux."""
    try:
        from serial.tools import list_ports  # optional

        return [(p.vid, p.pid, p.device) for p in list_ports.comports() if p.vid is not None]
    except ImportError:
        pass
    ports = []
    if sys.platform == "darwin":
        # A USB device's properties, then its children - the serial port's callout device among them.
        tree = subprocess.run(["ioreg", "-r", "-c", "IOUSBHostDevice", "-l", "-w0"], capture_output=True,
                              text=True).stdout
        vid = pid = None
        for line in tree.splitlines():
            if "<class IOUSBHostDevice" in line:
                vid = pid = None
            elif (m := re.search(r'"idVendor" = (\d+)', line)) and vid is None:
                vid = int(m.group(1))
            elif (m := re.search(r'"idProduct" = (\d+)', line)) and pid is None:
                pid = int(m.group(1))
            elif (m := re.search(r'"IOCalloutDevice" = "([^"]+)"', line)):
                ports.append((vid, pid, m.group(1)))
    else:
        for tty in glob.glob("/sys/class/tty/ttyACM*"):
            usb = os.path.realpath(os.path.join(tty, "device", ".."))
            try:
                vid = int(open(os.path.join(usb, "idVendor")).read(), 16)
                pid = int(open(os.path.join(usb, "idProduct")).read(), 16)
            except OSError:
                continue
            ports.append((vid, pid, "/dev/" + os.path.basename(tty)))
    return ports


def find_port():
    """The EH14's serial port: the one with its VID and PID (the current ones first, then the older firmware's),
    else the only USB modem that is not the bootloader's. Exits with a hint when there is none - the clock is on USB
    only while awake, with its menu open."""
    found = usb_serial_ports()
    for vid, pid in EH14_IDS:
        ports = [device for v, p, device in found if (v, p) == (vid, pid)]
        if len(ports) == 1:
            return ports[0]
    bootloader = {device for v, p, device in found if (v, p) in BOOTLOADER_IDS}
    ports = [p for p in glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/ttyACM*") if p not in bootloader]
    if len(ports) == 1:
        return ports[0]
    if not ports:
        sys.exit("No EH14 found - is it connected, and awake (its menu open)?")
    sys.exit("More than one serial port, pick the EH14's with --port: " + ", ".join(ports))

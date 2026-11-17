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

"""Sets the EH14's clock to this computer's time, with the set time command.

The clock has to be awake - open its menu first. It takes '%' and the six digits of HHMMSS (as numbers, not ASCII),
and sets the time once the line has been quiet for a second - so the time sent is a second ahead, sent right at the
turn of a second. The clock then announces the new time.

Usage: set_time.py [--port /dev/cu.usbmodemXXXX]
"""

import argparse
import os
import termios
import time

from eh14_serial import find_port

COMMAND_SET_TIME = 0x25
SESSION_TIMEOUT_S = 1.0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--port")
    args = parser.parse_args()
    port = args.port or find_port()

    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0  # raw: the digits go out as they are
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = attrs[5] = termios.B115200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

    # Right at the turn of a second, the time a second from now.
    time.sleep(1.0 - (time.time() % 1.0))
    when = time.localtime(time.time() + SESSION_TIMEOUT_S)
    digits = [int(c) for c in time.strftime("%H%M%S", when)]
    os.write(fd, bytes([COMMAND_SET_TIME] + digits))
    time.sleep(0.2)
    os.close(fd)
    print("Set to " + time.strftime("%H:%M:%S", when))


if __name__ == "__main__":
    main()

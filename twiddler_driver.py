#!/usr/bin/python3

import fcntl
import os
import struct
import termios
import threading
import tty
from time import sleep

def main():
    f = open('/dev/ttyS0', 'r+b', buffering=0)
    fd = f.fileno()

    tty.setraw(fd)

    iflag, oflag, cflag, lflag, ispeed, ospeed, cc = termios.tcgetattr(fd)

    cflag |= termios.CS8        # 8 bits
    cflag &= ~termios.PARENB    # no parity
    cflag &= ~termios.CSTOPB    # 1 stop bit

    cflag |= termios.CLOCAL     # ignore lack of DTR

    cc[termios.VMIN] = 1
    cc[termios.VTIME] = 0

    #baudrate = termios.B2400
    baudrate = termios.B2400

    termios.tcsetattr(fd, termios.TCSANOW,
                      [iflag, oflag, cflag, lflag, baudrate, baudrate, cc])

    # # What really sets the Twiddler communicating?
    # baudrate = 0                # Drop DTR

    # termios.tcsetattr(fd, termios.TCSANOW,
    #                   [iflag, oflag, cflag, lflag, baudrate, baudrate, cc])

    baudrate = termios.B2400
    cflag |= termios.CRTSCTS
    # cflag &= ~termios.CRTSCTS

    termios.tcsetattr(fd, termios.TCSANOW,
                      [iflag, oflag, cflag, lflag, baudrate, baudrate, cc])

    print(termios.TIOCM_RTS, hex(termios.TIOCM_RTS))
    print(termios.TIOCMBIC, hex(termios.TIOCMBIC))

    s = struct.pack('i', 0)
    s = fcntl.ioctl(fd, termios.TIOCMGET, s, True)
    print(s)

    # Not ours to set:
    # s = struct.pack('i', termios.TIOCM_CTS)
    # fcntl.ioctl(fd, termios.TIOCMBIC, s)
    # s = struct.pack('i', termios.TIOCM_DSR)
    # fcntl.ioctl(fd, termios.TIOCMBIS, s)

    s = struct.pack('i', termios.TIOCM_RTS)
    fcntl.ioctl(fd, termios.TIOCMBIC, s)
    s = struct.pack('i', termios.TIOCM_DTR)
    fcntl.ioctl(fd, termios.TIOCMBIC, s)

    # ?
    # s = struct.pack('i', termios.TIOCM_LE)
    # fcntl.ioctl(fd, termios.TIOCMBIS, s)

    s = struct.pack('i', 0)
    s = fcntl.ioctl(fd, termios.TIOCMGET, s, True)
    print(s)

    # baudrate = termios.B0
    # termios.tcsetattr(fd, termios.TCSANOW,
    #                   [iflag, oflag, cflag, lflag, baudrate, baudrate, cc])

    def check():
        sleep(0.1)
        s = struct.pack('i', 0)
        s = fcntl.ioctl(fd, termios.TIOCMGET, s, True)
        print(s)
        # s = struct.pack('i', termios.TIOCM_DTR)
        # fcntl.ioctl(fd, termios.TIOCMBIC, s)
        sleep(0.1)

    check()
    #os.system('stty -a < /dev/ttyS0')
    check()
    os.write(1, b'Ready\n')
    check()

    def tfunc():
        from time import sleep
        while True:
            sleep(1.0)
            s = struct.pack('i', 0)
            s = fcntl.ioctl(fd, termios.TIOCMGET, s, True)
            print(s)

    t = threading.Thread(target=tfunc)
    t.daemon = True
    t.start()

    def read():
        return ord(os.read(fd, 1))

    index = 0

    MOUSE_HIRES = 0x30
    MOUSE_LORES = 0x20

    # while True:
    #     print(repr(os.read(fd, 1)))

    for byte in read_bytes(fd):
        print(repr(byte))

    for block in read_blocks(read_bytes(fd)):
        #for block in (read_bytes(fd)):
        lower_7_bits = block[0] & 0x7f
        upper_6_bits = block[1] & 0x3f
        index = (upper_6_bits << 7) | lower_7_bits
        mouse_but = block[1]&0x40
        if not mouse_but:
            print([hex(b) for b in block]), hex(index), hex(mouse_but)

def read_blocks(bytes):
    for b in bytes:
        if b & 0x80:
            continue
        block = [b, next(bytes), next(bytes), next(bytes), next(bytes)]
        yield block
    return
    block = [b]
    while True:
        b = next(bytes)
        if b & 0x80:
            block.append(b)
        else:
            yield block
            block = [b]

def read_bytes(fd):
    while True:
        yield ord(os.read(fd, 1))


def foo():
    while True:
        b0 = read()
        while b0 & 0x80:
            b0 = read()
        b1 = read()
        index |= (b0 & 0x7f) | ((b1 & 0x3f) << 7)
        print(hex(index))
        mouse_but = b1 & 0x40
        if mouse_but:
            mouse_res = index & 0x30
            if mouse_res == MOUSE_HIRES:
                mres = 2
            elif mouse_res == MOUSE_LORES:
                mres = 1
            else:
                pass #mres = 0
            # mouse tilt

        #key_events_enabled = events

        del cc
        print(sorted(locals().items()))
        break

    while True:
        b0 = read()
        print(hex(b0))

if __name__ == '__main__':
    main()


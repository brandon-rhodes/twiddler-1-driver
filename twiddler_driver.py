#!/usr/bin/python3

# prior work: https://github.com/mati75/gpm/blob/master/debian/gpm.templates
# https://manpages.debian.org/jessie/gpm/gpm-types.7.en.html
# https://wiki.archlinux.org/index.php/serial_input_device_to_kernel_input

import fcntl
import os
import struct
import subprocess
import sys
import termios
import threading
import tty
from time import sleep

THUMBS = 'SHIFT NUM FUNC CTRL ALT MOUSE'.split()

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

    # s = struct.pack('i', termios.TIOCM_RTS)
    # fcntl.ioctl(fd, termios.TIOCMBIC, s)
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

    # check()
    #os.system('stty -a < /dev/ttyS0')
    # check()
    os.write(1, b'Ready\n')
    # check()

    def tfunc():
        from time import sleep
        while True:
            sleep(1.0)
            s = struct.pack('i', 0)
            s = fcntl.ioctl(fd, termios.TIOCMGET, s, True)
            print(s)

    # t = threading.Thread(target=tfunc)
    # t.daemon = True
    # t.start()

    def read():
        return ord(os.read(fd, 1))

    index = 0

    MOUSE_HIRES = 0x30
    MOUSE_LORES = 0x20

    # while True:
    #     print(repr(os.read(fd, 1)))

    # for byte in read_bytes(fd):
    #     print(repr(byte))

    mapping = {}

    with open('/home/brhodes/tabspace.ini') as ini:
        for line in ini:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            state, rhs = line.split('=', 1)
            thumbs, chord = state.rsplit(None, 1)
            thumbs = [t.strip() for t in thumbs.split('+')]
            text = parse_rhs(rhs)
            mapping[normalize(thumbs, chord)] = text

    print(mapping)

    for result in read_keys(mapping, read_bytes(fd)):
        print(repr(result))
        subprocess.check_call(['xdotool', 'type', result])

GLOBALS = {}
LOCALS = {
    'BACKSPACE': '\b',
    'ENTER': '\r',
    #'CTRLALTDEL'
    'ESCAPE': '\033',
    'TAB': '\t',
    'NULL': '\0',
    'CAPS_LOCK': 'TODO',
    'SCROLL_LOCK': 'TODO',
    'VT': '\v',
    'NUM_LOCK': 'TODO',
    #'X_TOGGLE'
}

def parse_rhs(text):
    r"""Parse the right-hand-side of a Twiddler ini entry.

    >>> parse_rhs(r'BACKSPACE,TAB,20,"abc","\"","\\"')
    '\x08\t\x14abc"\\'

    """
    try:
        value = eval(text, GLOBALS, LOCALS)
        if not isinstance(value, tuple):
            value = (value,)
        return ''.join(_parse_item(item) for item in value)
    except Exception:
        sys.stderr.write('Cannot parse right hand side: {}\n'.format(text))
        sys.exit(1)

def _parse_item(item):
    if isinstance(item, int):
        return chr(item)
    return item

def normalize(thumbs, chord):
    """Return a canonical string for the given Twiddler button state.

    >>> normalize(['NUM', 'func'], 'r0lm')
    'FUNC+NUM R0LM'

    """
    prefix = '+'.join(sorted(t.upper() for t in thumbs)) if thumbs else '0'
    return '{} {}'.format(prefix, chord.upper())

def read_keys(mapping, bytes):
    old_thumbs, old_chord = '', '0000'
    ready = False
    for new_thumbs, new_chord, vertical, horizontal in read_chords(bytes):
        if 'MOUSE' in new_thumbs:
            print('%5d %6d' % (vertical, horizontal))
            continue
        if new_chord.count('0') < old_chord.count('0'):
            ready = True        # they pressed another key
        elif ready and new_chord.count('0') > old_chord.count('0'):
            ready = False       # they released a key
            name = normalize(old_thumbs, old_chord)
            output = mapping.get(name)
            if output is not None:
                yield output
        old_thumbs = new_thumbs
        old_chord = new_chord

def read_chords(bytes):
    """Translate raw Twiddler packets into buttons, chords, and coordinates.

    >>> list(read_chords([0x47, 0xa5, 0x80, 0x87, 0xa9]))
    [(('NUM', 'ALT'), 'RL0R', 384, 1313)]

    """
    columns = '0LMR'
    thumb_bits = [(0x100 << i, name) for i, name in enumerate(THUMBS)]
    offsets = list(enumerate(range(0, 7*5, 7)))  # TODO: simplify

    for packet in read_packets(bytes):
        bits = sum((packet[i] & 0x7f) << offset for i, offset in offsets)
        chord = ''.join(columns[(bits >> i) & 0x3] for i in (0, 2, 4, 6))
        thumbs = tuple(thumb for bit, thumb in thumb_bits if bits & bit)
        vertical = (bits >> 14) & 0x1ff
        horizontal = bits >> 23
        yield thumbs, chord, vertical, horizontal

def read_packets(bytes):
    """Given a sequence of bytes, yield 5-byte Twiddler data packets."""
    bytes = iter(bytes)         # in case it's a list, so we can next() it
    for b in bytes:
        if b & 0x80:            # first byte of a packet has 8th bit unset
            continue
        yield b, next(bytes), next(bytes), next(bytes), next(bytes)

def read_bytes(fd):
    """Yield successive bytes from the given file descriptor."""
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
    if sys.version_info[0] < 3:
        sys.stderr.write('Error: run this script with Python 3\n')
        sys.exit(1)
    main()

# etethe hoi asrorywlm .1cfglkdcuwyebsriotnallxqpbvzorhethrorsanirorwxpdlkmhesrom lrn

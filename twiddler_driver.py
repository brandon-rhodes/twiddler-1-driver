#!/usr/bin/python3

import os
import termios
import tty

def main():
    f = open('/dev/ttyS0', 'r')
    fd = f.fileno()

    iflag, oflag, cflag, lflag, ispeed, ospeed, cc = termios.tcgetattr(fd)

    # cflag &= ~termios.CBAUD
    # cflag |= termios.B2400      # 2400 baud

    cflag |= termios.CS8        # 8 bits
    cflag &= ~termios.PARENB    # no parity
    cflag &= ~termios.CSTOPB    # 1 stop bit

    cflag |= termios.CLOCAL     # ignore lack of DTR
    #cflag &= ~termios.CBAUD     # drop DTR

    cc[termios.VMIN] = 1
    cc[termios.VTIME] = 0

    baudrate = termios.B2400

    termios.tcsetattr(fd, termios.TCSANOW,
                      [iflag, oflag, cflag, lflag, baudrate, baudrate, cc])

    tty.setraw(fd)

    os.system('stty -a < /dev/ttyS0')
    os.write(1, b'ready\n')
    while True:
        c = os.read(fd, 1)
        os.write(1, (repr(c) + '\n').encode('utf-8'))

if __name__ == '__main__':
    main()

import os
import sys
import argparse
import numpy
from matplotlib import pyplot

# constant
__version__ = '1.0'
__description__ = 'A command line tool to show raw image'
__epilog__ = 'Report bugs to <yehcj.tw@gmail.com>'

def main():
    parser = argparse.ArgumentParser(
        description = __description__,
        epilog = __epilog__
    )
    parser.add_argument('file', help='file name')
    parser.add_argument('--width', type=int, required=True)
    parser.add_argument('--height', type=int, required=True)
    parser.add_argument('--format', type=str, required=True)
    parser.add_argument(
        '-v', '-V', '--version',
        action='version',
        help='show version of program',
        version='v{}'.format(__version__)
    )
    args = parser.parse_args()

    pixel_cnt = args.width * args.height
    data = numpy.fromfile(args.file, dtype='uint8')

    if args.format == 'rgb':
        data = data[:pixel_cnt * 3]
        data = numpy.reshape(data, (args.height, args.width, 3))
    elif args.format == 'argb':
        data = data[:pixel_cnt * 4]
        a_index = [x * 4 for x in range(0, pixel_cnt)]
        data = numpy.delete(data, a_index)
        data = numpy.reshape(data, (args.height, args.width, 3))
    else:
        exit()

    pyplot.imshow(data)
    pyplot.show()

if __name__ == '__main__':
    main()
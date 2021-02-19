import subprocess
import sys
import os.path
import json
import fileinput
import enum
import argparse


class LoopDevice:
    def __init__(self):
        pass

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass


def update_image(parser: argparse, args):


def main():
    parser: argparse = argparse.ArgumentParser(description="disk img manager for project-dionysus",
                                               epilog="This script requires sudo")

    parser.add_argument('action', choices=['update', 'convert'])

    parser.add_argument('-f', '--file', type=argparse.FileType('rw'), nargs=1,
                        help="the disk image file",
                        required=True)
    parser.add_argument('-c', '--config', type=argparse.FileType('rw'), nargs=1,
                        help="the configuration file", required=True)
    parser.add_argument('-d', '--directory', type=argparse.FileType('rw'), nargs=1,
                        default=os.getcwd(),
                        help="the build directory", required=True)

    args = parser.parse_args()

    if args.action == 'update':
        update_image(parser, args)
    else:
        parser.error("Action {} is not yet implemented.".format(args.action))


if __name__ == "__main__":
    main()

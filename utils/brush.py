#!/usr/bin/env python2
from __future__ import print_function

import os.path

from PIL import Image
import numpy as np

try:
    import brush_pb2
except ImportError:
    print("failed to load brush protobuf", file=sys.stderr)
    print("run make (in utils/) and execute this script from within utils/",
          file=sys.stderr)

import google.protobuf.text_format


def validate_brush(brush):
    return len(brush.data) == brush.size**2


def show_info(brush):
    print("display name: {!r}".format(brush.display_name))
    print("license: {!r}".format(brush.license))
    print("size: {}".format(brush.size))
    if validate_brush(brush):
        print("valid")
    else:
        print("NOT valid")


def cmd_compile(args):
    args.display_name = (
        args.display_name
        or os.path.splitext(os.path.basename(args.infile.name))[0])

    with args.infile as f:
        img = Image.open(f)

        if img.size[0] != img.size[1]:
            print("image must be square")
            sys.exit(1)

        img = img.convert("F")
        data = np.asarray(img.getdata()) / img.getextrema()[1]
        if args.invert:
            data = 1. - data

        brush = brush_pb2.PixelBrushDef()
        brush.display_name = args.display_name
        brush.size = img.size[0]
        brush.license = args.license
        brush.data[:] = data

    with open(args.outfile, "wb") as f:
        f.write(brush.SerializeToString())

    if args.dump:
        print(google.protobuf.text_format.MessageToString(brush))


def cmd_decompile(args):
    brush = brush_pb2.PixelBrushDef()
    with args.infile as f:
        brush.ParseFromString(f.read())

    if args.show_info:
        show_info(brush)

    img = Image.new("F", (brush.size, brush.size))
    img.putdata(brush.data)
    if args.mode != "F":
        img = img.convert(args.mode)

    img.save(args.outfile)


def cmd_info(args):
    brush = brush_pb2.PixelBrushDef()
    with args.infile as f:
        brush.ParseFromString(f.read())

    show_info(brush)


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers()

    sp = subparsers.add_parser(
        "compile",
        help="Compile a brush from an image file")
    sp.set_defaults(func=cmd_compile)
    sp.add_argument(
        "--dump",
        help="Dump the protocol buffer as text after writing",
        action="store_true",
        default=False)
    sp.add_argument(
        "--display-name",
        help="Display name for the brush. Defaults to the basename of the input"
             " file",
        metavar="NAME",
        default=None)
    sp.add_argument(
        "--license",
        help="License for the source file. Defaults to CC0.",
        default="CC0",
        metavar="LICENSE")
    sp.add_argument(
        "--invert",
        help="Invert the image (so that black areas are strong)",
        default=False,
        action="store_true")
    sp.add_argument(
        "infile",
        help="Input file; this should be a square, monochrome image readable by"
             "PIL/Pillow",
        type=argparse.FileType("rb"))
    sp.add_argument(
        "outfile",
        help="Output file")

    sp = subparsers.add_parser(
        "decompile",
        help="Decompile a brush")
    sp.set_defaults(func=cmd_decompile)
    sp.add_argument(
        "--show-info",
        default=False,
        action="store_true",
        help="Also output the info from the info subcommand")
    sp.add_argument(
        "--format",
        default=None,
        help="Output format as understood by Pillow. If unset, Pillow will"
             " guess based on the file extension.")
    sp.add_argument(
        "--mode",
        default="L",
        help="Image mode, as understood by Pillow. Defaults to 'L' (8 bit "
             " greyscale)",
        metavar="MODE")
    sp.add_argument(
        "infile",
        type=argparse.FileType("rb"),
        help="Input file, which must be a brush")
    sp.add_argument(
        "outfile",
        help="Output file")

    sp = subparsers.add_parser(
        "info",
        help="Show information on a compiled brush")
    sp.set_defaults(func=cmd_info)
    sp.add_argument(
        "infile",
        type=argparse.FileType("rb"))

    args = parser.parse_args()

    args.func(args)

# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
import os
import site
import argparse
import math
import pathlib
from pathlib import Path
import logging as _logging

# ------------------------------------------------------------------------
_MODULENAME = 'ColorGrading.lut_compositor'

FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
_logging.basicConfig(level=_logging.DEBUG,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# ------------------------------------------------------------------------

# ------------------------------------------------------------------------
# set up access to OpenImageIO, within o3de or without
try:
    # running in o3de
    import azlmbr
    from ColorGrading.initialize import start
    start()
except Exception as e:
    # running external, start this module from:
    # Gems\AtomLyIntegration\CommonFeatures\Editor\Scripts\ColorGrading\cmdline\O3DE_py_cmd.bat
    pass

    try:
        _O3DE_DEV = Path(os.getenv('O3DE_DEV'))
        os.environ['O3DE_DEV'] = pathlib.PureWindowsPath(_O3DE_DEV).as_posix()
        _LOGGER.debug(_O3DE_DEV)
    except EnvironmentError as e:
        _LOGGER.error('O3DE engineroot not set or found')
        raise e

    try:
        _O3DE_BIN_PATH = Path(str(_O3DE_DEV))
        _O3DE_BIN = Path(os.getenv('O3DE_BIN', _O3DE_BIN_PATH.resolve()))
        os.environ['O3DE_BIN'] = pathlib.PureWindowsPath(_O3DE_BIN).as_posix()
        _LOGGER.debug(_O3DE_BIN)
        site.addsitedir(_O3DE_BIN)
    except EnvironmentError as e:
        _LOGGER.error('O3DE bin folder not set or found')
        raise e

try:
    import OpenImageIO as oiio
except ImportError as e:
    _LOGGER.error('OpenImageIO not found')
    raise e
# ------------------------------------------------------------------------

operations = {"composite":0, "extract":1}
invalidOp = -1

parser = argparse.ArgumentParser()
parser.add_argument('--i', type=str, required=True, help='input file')
parser.add_argument('--l', type=str, required=False, help='LUT to composite with screenshot in \'composite\' mode')
parser.add_argument('--op', type=str, required=True, help='operation. Should be \'composite\' or \'extract\'')
parser.add_argument('--s', type=int, required=True, help='size of the LUT (i.e. 16)')
parser.add_argument('--o', type=str, required=True, help='output file')
args = parser.parse_args()

op = operations.get(args.op, invalidOp)
if op == invalidOp:
    print("invalid operation")
    sys.exit(1)
elif op == 0:
    if args.l is None:
        print("no LUT file specified")
        sys.exit()

# read in the input image
inBuf = oiio.ImageBuf(args.i)
inSpec = inBuf.spec()
print("Input resolution is ", inBuf.spec().width, " x ", inBuf.spec().height)

if op == 0:
    outFileName = args.o
    print("writing %s..." % (outFileName))
    lutBuf = oiio.ImageBuf(args.l)
    lutSpec = lutBuf.spec()
    print("Resolution is ", lutBuf.spec().width, " x ", lutBuf.spec().height)
    if lutSpec.width != lutSpec.height*lutSpec.height:
        print("invalid input file dimensions. Expect lengthwise LUT with dimension W: s*s  X  H: s, where s is the size of the LUT")
        sys.exit(1)
    lutSize = lutSpec.height
    outSpec = oiio.ImageSpec(inSpec.width, inSpec.height, 3, oiio.TypeFloat)
    outBuf = oiio.ImageBuf(outSpec)
    outBuf.write(outFileName)
    for y in range(outBuf.ybegin, outBuf.yend):
        for x in range(outBuf.xbegin, outBuf.xend):
            srcPx = inBuf.getpixel(x, y)
            dstPx = (srcPx[0], srcPx[1], srcPx[2])
            if y < lutSpec.height and x < lutSpec.width:
                lutPx = lutBuf.getpixel(x, y)
                dstPx = (lutPx[0], lutPx[1], lutPx[2])
            outBuf.setpixel(x, y, dstPx)
    outBuf.write(outFileName)
elif op == 1:
    outFileName = args.o
    print("writing %s..." % (outFileName))
    lutSize = args.s
    lutSpec = oiio.ImageSpec(lutSize*lutSize, lutSize, 3, oiio.TypeFloat)
    lutBuf = oiio.ImageBuf(lutSpec)
    for y in range(lutBuf.ybegin, lutBuf.yend):
        for x in range(lutBuf.xbegin, lutBuf.xend):
            srcPx = inBuf.getpixel(x, y)
            dstPx = (srcPx[0], srcPx[1], srcPx[2])
            lutBuf.setpixel(x, y, dstPx)
    lutBuf.write(outFileName)
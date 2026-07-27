#!/usr/bin/env python3

# Opens a movie, waits, then repositions/stretches it - for reproducing
# the "new tile shows green until the decoder catches up" scenario.
# Usage:
#
#   python3 reposition_test.py /path/to/movie.mp4
#   python3 reposition_test.py /path/to/movie.mp4 --wait 40 \
#       --open 0 0 0.5 1 --new 0 0 1 1
#
# Run from a machine that can reach the running displaycluster instance's
# Python control port (DISPLAYCLUSTER_PYTHONPORT, default 1900 to match
# examples/run_script.py - check what your launch script actually set).

import argparse
import sys
import time

sys.path.insert(0, __file__.rsplit('/', 1)[0])
from DC import DC

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("uri", help="path to the movie file, as displaycluster itself sees it")
parser.add_argument("--host", default="localhost")
parser.add_argument("--port", type=int, default=1900)
parser.add_argument("--wait", type=float, default=30.0,
                     help="seconds to wait between opening and repositioning (default: 30)")
parser.add_argument("--open", nargs=4, type=float, metavar=("X", "Y", "W", "H"),
                     default=[0.0, 0.0, 0.5, 1.0],
                     help="initial x y w h, fractions of the wall (default: left half)")
parser.add_argument("--new", nargs=4, type=float, metavar=("X", "Y", "W", "H"),
                     default=[0.0, 0.0, 1.0, 1.0],
                     help="x y w h to reposition to (default: whole wall)")
args = parser.parse_args()

dc = DC(args.host, args.port)

ox, oy, ow, oh = args.open
print(f"opening {args.uri!r} at ({ox}, {oy}, {ow}, {oh})")
dc.open(args.uri, x=ox, y=oy, w=ow, h=oh)

print(f"waiting {args.wait}s...")
time.sleep(args.wait)

nx, ny, nw, nh = args.new
t0 = time.time()
print(f"repositioning to ({nx}, {ny}, {nw}, {nh})")
dc.reposition(args.uri, nx, ny, nw, nh)
print(f"reposition call returned after {time.time() - t0:.3f}s")

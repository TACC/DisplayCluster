#!/usr/bin/env python3

# Opens a movie, waits, then repositions/stretches it - for reproducing
# the "new tile shows green until the decoder catches up" scenario.
# Usage:
#
#   python3 reposition_test.py /path/to/movie.mp4
#   python3 reposition_test.py /path/to/movie.mp4 --wait 40 \
#       --open 0 0 0.5 1 --new 0 0 1 1
#
# Coordinates are in tile units, as everywhere in DC.py. Run from a machine
# that can reach the wall's remote API (port 1910 unless
# DISPLAYCLUSTER_API_PORT says otherwise), with its token (see DC.py).

import argparse
import sys
import time

sys.path.insert(0, __file__.rsplit('/', 1)[0])
from DC import DC

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("uri", help="path to the movie file, as displaycluster itself sees it")
parser.add_argument("--host", default="localhost")
parser.add_argument("--port", type=int, default=None,
                    help="API port (default: $DISPLAYCLUSTER_API_PORT, else 1910)")
parser.add_argument("--wait", type=float, default=30.0,
                     help="seconds to wait between opening and repositioning (default: 30)")
parser.add_argument("--open", nargs=4, type=float, metavar=("X", "Y", "W", "H"),
                     default=[0.0, 0.0, 0.5, 1.0],
                     help="initial x y w h, in tiles (default: 0 0 0.5 1)")
parser.add_argument("--new", nargs=4, type=float, metavar=("X", "Y", "W", "H"),
                     default=[0.0, 0.0, 1.0, 1.0],
                     help="x y w h to reposition to, in tiles (default: 0 0 1 1)")
args = parser.parse_args()

dc = DC(args.host, args.port)

ox, oy, ow, oh = args.open
print(f"opening {args.uri!r} at ({ox}, {oy}, {ow}, {oh})")
name = dc.open(args.uri, x=ox, y=oy, w=ow, h=oh)

print(f"waiting {args.wait}s...")
time.sleep(args.wait)

nx, ny, nw, nh = args.new
t0 = time.time()
print(f"repositioning to ({nx}, {ny}, {nw}, {nh})")
dc.reposition(name, nx, ny, nw, nh)
print(f"reposition call returned after {time.time() - t0:.3f}s")

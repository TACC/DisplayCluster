#!/usr/bin/env python3

# Sets the wall's sensors from the keyboard, and shows them as the wall does
# in the red/green layout: columns of two cells, one above the other, where
# each cell is a red window and a green one stacked, green while its sensor
# is lit and red while it isn't. Sensor n is in column n // 2, on top for
# even n and below for odd.
#
# Usage: python3 sensors.py [wall host]     (default: localhost)
#
# Commands:
#   s N on|off   set sensor N
#   t N          toggle sensor N
#   a on|off     set every sensor
#   setup        clear the wall and lay out the red/green windows
#   (blank)      show the sensors again (another client may have changed them)
#   q            quit
#
# Setting sensors needs the wall's token but not control of it. setup does
# take control, and gives it back when it's done. The wall offers as many
# sensors as its DISPLAYCLUSTER_SENSORS says.

import os
import sys

here = os.path.dirname(os.path.abspath(__file__))

# DC.py is in python/ beside this directory, both in the repo and when installed
sys.path.insert(0, os.path.join(here, '..', 'python'))
from DC import DC

try:
    import readline     # line editing and history, where available
except ImportError:
    pass

# where the wall finds the images for setup: the copies in the container image
RED = '/usr/local/src/DisplayCluster/examples/red.png'
GREEN = '/usr/local/src/DisplayCluster/examples/green.png'

COLOR = sys.stdout.isatty()

def cell(n, lit):
    label = ' %2d %s ' % (n, 'on ' if lit else 'off')
    if not COLOR:
        return '[' + label + ']'
    return ('\033[42;30m' if lit else '\033[41;97m') + label + '\033[0m'

def show(value, count):
    columns = (count + 1) // 2
    for row in (0, 1):
        cells = []
        for column in range(columns):
            n = column * 2 + row
            if n < count:
                cells.append(cell(n, value >> n & 1))
        print('  ' + '  '.join(cells))
    print('  value 0x%02x' % value)

# a red window over a green one in each cell, both following its sensor
def setup(dc, count):
    tilesWide, tilesHigh = dc.getConfiguration()
    columns = (count + 1) // 2
    w = tilesWide / columns
    h = tilesHigh / 2
    pad = 0.01 * min(w, h)
    dc.clear()
    for n in range(count):
        x = (n // 2) * w + pad
        y = (n % 2) * h + pad
        for uri, showWhenLit in ((RED, False), (GREEN, True)):
            dc.open(uri, x, y, w - 2 * pad, h - 2 * pad, name = 'sensor %d %s' % (n, 'green' if showWhenLit else 'red'),
                    sensorMask = 1 << n, showWhenLit = showWhenLit)
    dc.releaseControl()

def sensor(word, count):
    n = int(word)
    if not 0 <= n < count:
        raise ValueError('sensors are 0 to %d' % (count - 1))
    return n

def onoff(word):
    if word not in ('on', 'off'):
        raise ValueError('on or off, not %r' % word)
    return word == 'on'

dc = DC(sys.argv[1] if len(sys.argv) > 1 else 'localhost', label = 'sensors.py', strict = True)

count = dc.getSensorCount()
if count == 0:
    sys.exit('the wall offers no sensors: start it with DISPLAYCLUSTER_SENSORS set')

show(dc.getSensors(), count)

while True:
    try:
        words = input('sensors> ').split()
    except (EOFError, KeyboardInterrupt):
        print()
        break

    try:
        if not words:
            value = dc.getSensors()
        elif words[0] in ('q', 'quit', 'exit'):
            break
        elif words[0] == 's' and len(words) == 3:
            n = sensor(words[1], count)
            value = dc.setSensors(onoff(words[2]) << n, mask = 1 << n)
        elif words[0] == 't' and len(words) == 2:
            n = sensor(words[1], count)
            value = dc.setSensors(~dc.getSensors() & 1 << n, mask = 1 << n)
        elif words[0] == 'a' and len(words) == 2:
            every = (1 << count) - 1
            value = dc.setSensors(every if onoff(words[1]) else 0, mask = every)
        elif words[0] == 'setup':
            setup(dc, count)
            value = dc.getSensors()
        else:
            print('  s N on|off, t N, a on|off, setup, or q')
            continue
    except ValueError as e:
        print('  ' + str(e))
        continue
    except Exception as e:
        print('  the wall said: %s' % e)
        continue

    show(value, count)

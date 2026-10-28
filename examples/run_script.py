#!/usr/bin/env python3

# Plays examples/script on the wall: three images opening and closing over
# ten seconds.
#
# Usage: python3 run_script.py [wall host]     (default: localhost)
#
# The images' paths in the script are where they are inside the container
# (/usr/local/examples); for a native install, change them to the install
# prefix's examples/ directory.

import os
import sys

here = os.path.dirname(os.path.abspath(__file__))

# DC.py is in python/ beside this directory, both in the repo and when installed
sys.path.insert(0, os.path.join(here, '..', 'python'))
from DC import DC

dc = DC(sys.argv[1] if len(sys.argv) > 1 else 'localhost', label = 'run_script.py')
dc.run_events(dc.create_event_list(os.path.join(here, 'script')))

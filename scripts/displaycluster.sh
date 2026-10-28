#!/bin/sh
# Runs one rank of the wall inside the container: the command it's given
# (startdisplaycluster passes displaycluster.py), with the image's libraries
# first on the library path.
LD_LIBRARY_PATH=/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} exec "$@"

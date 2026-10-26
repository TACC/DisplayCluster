#!/bin/bash
# Runs a native macOS build of DisplayCluster (cmake -S . -B build) as a small
# wall on this Mac: one MPI rank for the control window plus one per process
# in the configuration.
#
# Usage: scripts/run-macos.sh [configuration.json]
#        (default config/configuration_macos_fullscreen.json: one full-screen
#        tile on the external 1920x1080 display, to the right of the built-in
#        one; config/configuration_macos.json is three 400x400 windowed tiles)
#
# MPI is the Open MPI the build was configured with (DISPLAYCLUSTER_MPIRUN,
# default ~/local/openmpi-4.1/bin/mpirun - Homebrew's Open MPI 5 doesn't start
# here), over shared memory only: every rank is on this Mac, and leaving Open
# MPI to try TCP over Wi-Fi or a VPN can hang it.
#
# The remote API listens on localhost only. It uses ~/.displaycluster/api_token
# if there is one, and HTTPS only with both ~/.displaycluster/api_cert.pem and
# api_key.pem. Anything already set in the environment wins.

DC=$(cd "$(dirname "$0")/.." && pwd)

export DISPLAYCLUSTER_DIR=$DC
export DISPLAYCLUSTER_CONFIG=${1:-${DISPLAYCLUSTER_CONFIG:-$DC/config/configuration_macos_fullscreen.json}}
export DISPLAYCLUSTER_API_BIND=${DISPLAYCLUSTER_API_BIND:-127.0.0.1}
export DISPLAYCLUSTER_SENSORS=${DISPLAYCLUSTER_SENSORS:-6}
export DISPLAYCLUSTER_TIMEOUT=${DISPLAYCLUSTER_TIMEOUT:-10800}
export DISPLAYCLUSTER_SCREENSAVER_IMAGE=${DISPLAYCLUSTER_SCREENSAVER_IMAGE:-$DC/examples/tacc-logo.png}

MPIRUN=${DISPLAYCLUSTER_MPIRUN:-$HOME/local/openmpi-4.1/bin/mpirun}

PROCESSES=$(python3 -c "import json, sys; print(len(json.load(open(sys.argv[1]))['processes']))" "$DISPLAYCLUSTER_CONFIG") || exit 1

# pass every DISPLAYCLUSTER_* variable on to the ranks, e.g. DISPLAYCLUSTER_FRAME_STATS=1
FORWARD=()
for v in $(compgen -e | grep '^DISPLAYCLUSTER_'); do
    FORWARD+=(-x "$v")
done

exec "$MPIRUN" -np $((PROCESSES + 1)) --mca btl self,vader "${FORWARD[@]}" "$DC/build/displaycluster"

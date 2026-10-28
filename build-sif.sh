#!/bin/bash
# Builds displaycluster.sif from this checkout, uncommitted changes and all:
# a Docker image first, then the apptainer image converted from it.

# stop at the first failure: each step needs the one before
set -e

cd "$(dirname "$0")"

# the image as an archive for apptainer to read: about 2.5 GB, so it goes in
# a directory of its own and is deleted however the script ends
work=$(mktemp -d "${TMPDIR:-/tmp}/displaycluster-build.XXXXXX")
trap 'rm -rf "$work"' EXIT

docker build --label org.tacc.displaycluster=1 -f docker/Dockerfile -t displaycluster .

# the images earlier builds left behind when this one took their tag
docker image prune -f --filter label=org.tacc.displaycluster=1 > /dev/null

docker save displaycluster:latest -o "$work/displaycluster.tar"
rm -f displaycluster.sif
apptainer build displaycluster.sif docker-archive:"$work/displaycluster.tar"

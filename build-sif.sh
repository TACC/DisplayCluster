#!/bin/bash
# Builds a DisplayCluster image from this checkout, uncommitted changes and
# all: a Docker image first, then the apptainer image converted from it.
#
#   build-sif.sh [-r root] [-k count]
#
# Given a DisplayCluster directory (-r, or $DISPLAYCLUSTER_ROOT), it builds the
# image straight into the directory's images/, as
# displaycluster-<date>-<commit>.sif, then installs it there with
# scripts/install-displaycluster: the wall runs it from its next start, and
# the scripts and DC.py that came with it are installed in bin/ and python/.
# -k is how many images to keep (default 3). Without a directory, it writes
# displaycluster.sif in this checkout.
#
# Building needs Docker. If the machine with Docker can't see the wall's
# directory, build into a local one and copy it over, or install the image
# there with install-displaycluster.

# stop at the first failure: each step needs the one before
set -e

ROOT=${DISPLAYCLUSTER_ROOT:-}
KEEP=3

while getopts r:k:h opt; do
    case $opt in
        r) ROOT=$OPTARG ;;
        k) KEEP=$OPTARG ;;
        *) sed -n '5s/^# \{0,1\}//p' "$0" >&2; exit 2 ;;
    esac
done

cd "$(dirname "$0")"

# the image as an archive for apptainer to read: about 2.5 GB, so it goes in
# a directory of its own and is deleted however the script ends
work=$(mktemp -d "${TMPDIR:-/tmp}/displaycluster-build.XXXXXX")
partial=
trap 'rm -rf "$work" "$partial"' EXIT

docker build --label org.tacc.displaycluster=1 -f docker/Dockerfile -t displaycluster .

# the images earlier builds left behind when this one took their tag
docker image prune -f --filter label=org.tacc.displaycluster=1 > /dev/null

docker save displaycluster:latest -o "$work/displaycluster.tar"

if [ -z "$ROOT" ]; then
    rm -f displaycluster.sif
    apptainer build displaycluster.sif docker-archive:"$work/displaycluster.tar"
    exit
fi

# named for when it was built and what from: "modified" if there were
# uncommitted changes to tracked files, since those are built in too
commit=$(git rev-parse --short HEAD 2> /dev/null || echo unknown)
if [ "$commit" != unknown ] && [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    commit=$commit-modified
fi
name=displaycluster-$(date +%Y%m%d-%H%M)-$commit.sif

mkdir -p "$ROOT/images"

# built under a temporary name, so a failed build never leaves a broken image
partial=$ROOT/images/.$name.partial
apptainer build "$partial" docker-archive:"$work/displaycluster.tar"
mv "$partial" "$ROOT/images/$name"
partial=

scripts/install-displaycluster -r "$ROOT" -k "$KEEP" "$ROOT/images/$name"

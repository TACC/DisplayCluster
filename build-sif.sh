#!/bin/bash
# stop at the first failure: each step needs the one before
set -e

docker build -f docker/Dockerfile -t displaycluster .
docker save displaycluster:latest -o /tmp/displaycluster.tar
if test -e displaycluster.sif ; then
  rm -rf displaycluster.sif
fi
apptainer build displaycluster.sif docker-archive:/tmp/displaycluster.tar

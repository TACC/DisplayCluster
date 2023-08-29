# Instructions

These instructions detail how to get displaycluster working for a linux based cluster, as well as building and modifying the container if necessary.

There are two base containers used for creating displaycluster: a modified Ubuntu-20 container, and the DisplayCluster container. The Ubuntu-20 container has a couple of extra installs for allowing the containers to gain access to the windowing system. The Displaycluster container builds and runs DisplayCluster within a modified Ubuntu image. How to setup on a new system is detailed below.

## Setup

1. Create a directory in which all associated displaycluster content will live.

```sh
mkdir dc
cd dc
```

2. clone the displaycluster repository and pull the displaycluster container from docker using apptainer
```sh
git clone https://github.com/TACC/DisplayCluster.git
apptainer pull docker://ajs51210/displaycluster
```

## Development
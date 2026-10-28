# Setting up DisplayCluster: from GitHub to a working wall

This guide takes you from nothing to a running wall. It assumes you know your
way around a Linux shell, but nothing about DisplayCluster.

- [How DisplayCluster works](#how-displaycluster-works)
- [Choosing a build](#choosing-a-build)
- [What every wall needs](#what-every-wall-needs)
- [Getting the code](#getting-the-code)
- [Container build](#container-build)
- [Native build](#native-build)
- [Preparing the display nodes](#preparing-the-display-nodes)
- [Describing the wall: configuration.json](#describing-the-wall-configurationjson)
- [The launcher: startdisplaycluster](#the-launcher-startdisplaycluster)
- [First run](#first-run)
- [Using the control window](#using-the-control-window)
- [Remote control and the web UI](#remote-control-and-the-web-ui)
- [Showing a screen or a browser tab](#showing-a-screen-or-a-browser-tab)
- [Updating](#updating)
- [Troubleshooting](#troubleshooting)
- [Environment variable reference](#environment-variable-reference)

## How DisplayCluster works

A DisplayCluster wall is one MPI job:

- **Rank 0** is the control process. It runs on the machine you start the wall
  from (the *head node*). It owns the wall's state: which windows are open and
  where they are. It shows a small control window, and it serves:
  - the remote-control API and web UI, on TCP port 1910;
  - pixel streams from DesktopStreamer, on TCP port 1701.
- **Ranks 1 and up** are the display processes, one for each entry in the
  wall's configuration file. Each one runs on a display node, opens full-screen
  windows on that node's X display, and draws its part of the wall.

```
              browser / phone / DC.py / DesktopStreamer
                               |
                     port 1910 | port 1701
                               v
   head node   [ rank 0: control window, API, web UI ]
                               |
                    MPI over the local network
              +----------------+----------------+
              v                v                v
   node 1  [ rank 1 ]     node 2 [ rank 2 ]   ...   one rank per "process"
           X display :0          X display :0       in configuration.json
           tiles (0,0) (1,0)     tiles (2,0) (3,0)
```

Every rank reads the same configuration file and the same media files, so
both have to be at the same path on every machine. Usually that's a shared
home directory.

## Choosing a build

There are two ways to build and run it. Both produce the same program.

| | Container (apptainer) | Native |
|---|---|---|
| What you build | one image file, `displaycluster.sif` | a `displaycluster` executable |
| Build machine needs | Docker and apptainer | every library in the [dependency list](#dependencies) |
| Wall nodes need | apptainer, the NVIDIA driver, and Open MPI matching the image's | every runtime library, on every node |
| Updating | copy one file | reinstall on every node |
| Best for | production walls, from one machine up | development, or a Mac |

**Use the container for a real wall.** The rest of this guide covers both.
Where they differ, it says so.

## What every wall needs

**Hardware**
- One head node, for the control process. It can also run display
  processes. The smallest wall is a single machine running the control process
  and one display process, which drives all of that machine's monitors. That's
  a complete setup in its own right, not just a test, and it needs no SSH or
  MPI networking between machines.
- One or more display nodes with NVIDIA GPUs. Other GPUs work with a native
  build, but the container image is built for NVIDIA.
- A network between them. Gigabit works; faster helps with movies and streams.

**Software, on every machine**
- Linux with X11. Rocky 9 and Ubuntu 22.04/24.04 are known to work. The
  display processes draw through X; Wayland isn't supported.
- The NVIDIA driver, with an X server running on each display node (see
  [Preparing the display nodes](#preparing-the-display-nodes)).
- Open MPI 4.1. For the container, the host's Open MPI must be 4.1.x to match
  the image's (4.1.2 from Ubuntu 22.04). Rocky 9's 4.1.1 works.
- For the container: apptainer (1.1 or later) on every machine. Docker is
  needed only on the machine that builds the image.
- Python 3 on the head node. The launcher uses it to read the configuration.

**Accounts and access**
- One account that runs the wall, with the same home directory (or at least
  the same paths) on every machine.
- Passwordless SSH from the head node to every display node, as that account.
  MPI starts the remote ranks over SSH. See [Security](#security) for how to
  set it up safely.
- Permission for that account to draw on each display node's X display.

**Network ports**
- Between the wall's machines: MPI uses arbitrary TCP ports, so don't firewall
  traffic between them.
- Into the head node from users' machines:
  - **1910**: the web UI and remote API.
  - **1701**: DesktopStreamer, if you use it. This port has no token, so
    anyone who can reach it can put a window on the wall. Open it only to
    networks you trust.

The browser extension uses port 1910, like the web UI.

### Security

A wall needs a few openings that a normal server wouldn't have. Keep each one
as narrow as you can.

**SSH between the nodes.** "Passwordless" still means authenticated: MPI
logs in with an SSH key, so only someone holding the private key gets in. The
key has no passphrase, though, because `mpirun` can't type one. Anyone who
gets into the wall's account on the head node therefore gets into every
display node as well. To limit what that exposes:

- **Run the wall from a dedicated account,** not root and not a person's own
  account. A stolen key then reaches only the wall.
- **Give the wall its own key.** Don't reuse anyone's personal key. As the
  wall's account:
  ```bash
  ssh-keygen -t ed25519 -N '' -C 'displaycluster wall' -f ~/.ssh/id_wall
  ```
  and add `IdentityFile ~/.ssh/id_wall` for the display nodes in
  `~/.ssh/config`.
- **Accept the key only from the head node.** In `~/.ssh/authorized_keys` on
  the display nodes, start its line with a `from=` restriction. With a shared
  home directory this is one file for every node:
  ```
  from="headnode.example.edu,10.0.0.1" ssh-ed25519 AAAA... displaycluster wall
  ```
- **Keep the display nodes on a private network** if you can, reachable only
  from the head node. They then don't need SSH open to anything else.

**The X servers.** `xhost +si:localuser:<account>` lets in only that one
local account, which is fine. Starting X with `-ac` instead turns off access
control: anyone who can reach the node can then draw on its screens or read
its input. Use `-ac` only on nodes that run nothing else and that only the
head node can reach. `-nolisten tcp`, in the `X.service` example, keeps X
off the network either way.

**The remote API, port 1910.** Give it a token, and a certificate so the token
isn't sent in the clear (see [Remote control](#remote-control-and-the-web-ui)).
Without a token it listens only on the head node itself. Anyone with the token
can control the wall, so treat it like a password, and change it if it leaks:
write a new one to `~/.displaycluster/api_token` and restart the wall.

**DesktopStreamer, port 1701.** There's no token and no encryption, so anyone
who can reach the port can put a window on the wall, and anyone on the network
path can see what's streamed. Open it only to networks you trust, or not at
all if presenters can use the browser extension, which goes through port 1910
with the token.

## Getting the code

```bash
git clone https://github.com/TACC/DisplayCluster.git
cd DisplayCluster
```

The default branch, `main`, is the current version. The directories you'll
meet in this guide:

| Path | What's there |
|---|---|
| `src/` | the C++ source |
| `docker/Dockerfile` | the container recipe |
| `build-sif.sh` | builds the container image |
| `scripts/startdisplaycluster` | the launcher you copy and edit for your wall |
| `scripts/displaycluster.sh`, `displaycluster.py` | run each rank. The launcher calls them; you don't edit them. |
| `config/`, `examples/configuration.json` | example wall configurations |
| `ui/` | the web UI |
| `python/DC.py` | Python client for the remote API |
| `streamer/` | DesktopStreamer, a separate program for presenters' machines |
| `extension/` | Chrome/Edge extension that streams a browser tab to the wall |
| `doc/remote-control.md` | the remote API, web UI, tokens and HTTPS in detail |

## Container build

You build the image once, on any Linux machine with Docker and apptainer. It
doesn't have to be part of the wall. Then you copy the one `.sif` file to
where the wall can read it.

### 1. Install Docker and apptainer on the build machine

- **Docker:** follow docker.com's instructions for your distribution. Your
  account must be able to run `docker` (usually by being in the `docker`
  group). Check with `docker run --rm hello-world`.
- **Apptainer:** on Rocky/RHEL 9, `sudo dnf install epel-release && sudo dnf
  install apptainer`. On Ubuntu, use the `.deb` from the apptainer GitHub
  releases page.

The build needs roughly 15 GB of free space for Docker's layers and the
image, plus room in `/tmp` for a 1 GB archive.

### 2. Build

From the top of the checkout (not from inside `docker/`):

```bash
./build-sif.sh
```

This runs three steps and stops at the first one that fails:

1. `docker build -f docker/Dockerfile -t displaycluster .` compiles
   DisplayCluster inside an Ubuntu 22.04 image with Qt 6, Boost, Open MPI,
   FFmpeg and the CUDA driver API. The image is based on NVIDIA's
   `nvidia/opengl` image. It copies your checkout, including any uncommitted
   changes, so what you build is what's in the directory.
2. `docker save` writes the image to `/tmp/displaycluster.tar`.
3. `apptainer build` converts that archive to `displaycluster.sif` in the
   checkout. Any old `displaycluster.sif` there is deleted first.

The first build downloads and installs a lot and takes 15–30 minutes. Later
builds reuse Docker's cache and take a few minutes. The result is about
900 MB.

Check that it worked:

```bash
ls -l displaycluster.sif                       # today's date
apptainer exec displaycluster.sif ls /usr/local/bin/displaycluster
```

### 3. Put the image where the wall can read it

Every machine in the wall must be able to read the `.sif` at the same path.
On a shared filesystem, leave it where it is or copy it somewhere permanent.
Otherwise, copy it to the same path on every node.

### What's inside the image

| Path in the image | What it is |
|---|---|
| `/usr/local/bin/displaycluster` | the program |
| `/usr/local/ui`, `/usr/local/data`, `/usr/local/examples` | web UI, images, examples |
| `/usr/local/src/DisplayCluster` | the source it was built from, including `scripts/` |

The host's NVIDIA driver is mapped into the container at run time
(`apptainer run --nv`), so the image works with whatever driver version the
nodes have. Apptainer also makes your home directory and the current
directory visible inside the container. That's how the display processes see
your configuration file, media and `~/.displaycluster`.

Now go to [Preparing the display nodes](#preparing-the-display-nodes).

## Native build

A native build compiles DisplayCluster directly on the machine. For a wall,
every display node then needs the same libraries installed.

### Dependencies

| Dependency | Needed for | Required? |
|---|---|---|
| C++17 compiler (GCC 9+, Clang 10+, MSVC 2022) | everything | yes |
| CMake 3.16+ | building | yes |
| pkg-config | finding libjpeg-turbo on Linux | yes, on Linux |
| Qt 6: Core, Gui, Widgets, Network, Svg, Xml, Concurrent, OpenGL | windows, drawing, the network | yes |
| Boost: serialization, date_time, iostreams, plus the header-only algorithm, tokenizer, smart_ptr | sending state between ranks | yes |
| Open MPI 4.1, built with `MPI_THREAD_MULTIPLE` support (the packaged ones are) | running across machines | yes |
| FFmpeg: libavcodec, libavformat, libavutil, libswscale | movies | yes |
| libjpeg-turbo (the `turbojpeg` API) | pixel streams, thumbnails | yes |
| OpenGL and GLU | drawing | yes |
| OpenSSL 3 | HTTPS for the remote API | optional; without it the API is HTTP only |
| CUDA driver API (`cuda.h`, `libcuda`) | hardware movie decode on NVIDIA GPUs | optional; turn off with `-DDISPLAYCLUSTER_CUDA=OFF` |
| NVIDIA driver | running on NVIDIA GPUs | at run time |
| Python 3 | the launcher; `DC.py` | at run time |

JSON parsing (`src/json.hpp`) and the HTTP server (`src/httplib.h`) are
included in the source, so they need no packages.

### Ubuntu 22.04 / 24.04

```bash
sudo apt install build-essential cmake pkg-config git \
    qt6-base-dev libqt6svg6-dev libqt6opengl6-dev qt6-qpa-plugins \
    libboost-dev libboost-serialization-dev libboost-date-time-dev libboost-iostreams-dev \
    libopenmpi-dev openmpi-bin \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev \
    libturbojpeg0-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    libssl-dev
```

On 24.04, `qt6-svg-dev` also works in place of `libqt6svg6-dev`.

For hardware movie decode, add NVIDIA's CUDA repository and install just the
driver-API headers and stub library. You don't need the whole toolkit.

```bash
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb && sudo apt update
apt-cache search cuda-cudart-dev       # pick the newest, e.g.
sudo apt install cuda-cudart-dev-13-3
```

Use `ubuntu2404` in the URL on 24.04. Or skip this and configure with
`-DDISPLAYCLUSTER_CUDA=OFF`, which decodes movies on the CPU.

### Rocky / RHEL 9

This list hasn't been tested as a whole on a clean machine. The wall at TACC
runs the container. Qt 6 comes from EPEL. FFmpeg comes from RPM Fusion,
because RHEL's own packages leave out most codecs.

```bash
sudo dnf install epel-release
sudo dnf config-manager --set-enabled crb
sudo dnf install --nogpgcheck https://mirrors.rpmfusion.org/free/el/rpmfusion-free-release-9.noarch.rpm
sudo dnf install gcc-c++ cmake pkgconf-pkg-config git \
    qt6-qtbase-devel qt6-qtsvg-devel \
    boost-devel \
    openmpi-devel \
    ffmpeg-devel \
    libjpeg-turbo-devel turbojpeg turbojpeg-devel \
    mesa-libGL-devel mesa-libGLU-devel \
    openssl-devel
module load mpi/openmpi-x86_64         # puts mpicc and mpirun on the PATH
```

`turbojpeg` is a separate package from `libjpeg-turbo-devel`. Without it,
CMake fails with "file does not exist" on the imported target. For CUDA,
install `cuda-cudart-devel-13-*` from NVIDIA's `rhel9` repository, or
configure with `-DDISPLAYCLUSTER_CUDA=OFF`.

### macOS

macOS runs a small wall in windows on one Mac. That's useful for development.
There's no CUDA, so movies decode on the CPU and CUDA is off by default.

```bash
brew install cmake qt boost ffmpeg jpeg-turbo openssl@3
```

Homebrew's Open MPI 5 fails in `MPI_Init` on macOS. Build Open MPI 4.1 from
source instead:

```bash
curl -LO https://download.open-mpi.org/release/open-mpi/v4.1/openmpi-4.1.6.tar.bz2
tar xf openmpi-4.1.6.tar.bz2 && cd openmpi-4.1.6
./configure --prefix=$HOME/local/openmpi-4.1 && make -j8 install
```

Then configure with it and Homebrew's Qt:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix)" \
      -DMPI_CXX_COMPILER=$HOME/local/openmpi-4.1/bin/mpicxx
cmake --build build -j8
scripts/run-macos.sh
```

`scripts/run-macos.sh` runs the build as a wall of one full-screen tile on an
external display. Pass `config/configuration_macos.json` for three small
windows instead. Its comments list the settings it uses.

### Windows

See the Windows section of the [README](../README.md#windows-11). It uses
Visual Studio 2022, vcpkg (`vcpkg.json` lists the packages), MS-MPI, and a
prebuilt FFmpeg.

### Configure, build, install (Linux)

Build outside the source tree. The container build copies the whole checkout,
and a `build/` inside it gets dragged along by accident.

```bash
cmake -S . -B ../dc-build -DCMAKE_INSTALL_PREFIX=/opt/displaycluster
cmake --build ../dc-build -j$(nproc)
sudo cmake --install ../dc-build
```

Things to check in CMake's output:
- `remote API: HTTPS supported (OpenSSL 3.x)`. If it says "HTTP only",
  install the OpenSSL development package and configure again.
- `Building without CUDA` appears only if you turned CUDA off.

The install puts `bin/displaycluster`, `ui/`, `data/`, `examples/`,
`python/` and `doc/` under the prefix. Set `DISPLAYCLUSTER_DIR` to that
prefix when running. The program looks there for `data/` and `ui/`, and
refuses to start without it. Install to the same path on every display node,
or to a shared filesystem.

The default build type is `RelWithDebInfo`: optimized, but with debugging
symbols, so a crash gives a useful backtrace.

### Running a native build

The launcher in `scripts/` starts the container. For a native build, run
`mpirun` directly:

```bash
export DISPLAYCLUSTER_DIR=/opt/displaycluster
export DISPLAYCLUSTER_CONFIG=$HOME/wall.json
export DISPLAYCLUSTER_TIMEOUT=3600
hosts=localhost,$(python3 -c 'import json,sys; print(",".join(p["host"] for p in json.load(open(sys.argv[1]))["processes"]))' $DISPLAYCLUSTER_CONFIG)
mpirun -host $hosts $(for v in $(compgen -e | grep ^DISPLAYCLUSTER_); do echo -x $v; done) \
    $DISPLAYCLUSTER_DIR/bin/displaycluster
```

Each display process opens its windows on the X display named in its entry
in the configuration file. If you start `displaycluster` directly like this,
set `DISPLAY` on each node yourself, or start it through
`$DISPLAYCLUSTER_DIR/bin/displaycluster.py`, which sets it from the
configuration file.

## Preparing the display nodes

The display processes need an X server, running on the NVIDIA driver, that
the wall's account can draw on. Most first-run problems come from this.

### 1. NVIDIA driver

Install the driver from your distribution or NVIDIA. Check that `nvidia-smi`
lists every GPU.

### 2. Run X as a service, not a desktop

On a dedicated display node, nobody logs in at the screen. Run a bare X server
from systemd, and don't run a login manager. If GDM, LightDM or SDDM is
running, it grabs the GPUs and your X server fails with "Permission denied"
on `/dev/dri/...` and "no screens found".

```bash
sudo systemctl disable --now gdm          # or lightdm / sddm
sudo systemctl set-default multi-user.target
```

A minimal `/etc/systemd/system/X.service`:

```ini
[Unit]
Description=X server for the display wall
After=systemd-user-sessions.service

[Service]
ExecStart=/usr/bin/Xorg :0 -nolisten tcp -noreset vt1
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload && sudo systemctl enable --now X
```

Restart it after any change to the X configuration with
`sudo systemctl restart X`.

### 3. Let the wall's account draw on it

The X server above runs as root. The simplest way to let the wall's account
use it is, for a single user:

```bash
DISPLAY=:0 xhost +si:localuser:vislab      # as root, after X starts
```

Or, on a node that does nothing else, add `-ac` to the `Xorg` line above to
turn off access control altogether (but see [Security](#security)). Check from the wall's account with
`DISPLAY=:0 xdpyinfo | head`.

### 4. Turn off screen blanking and DPMS

With DPMS on, X turns the monitors off after a while. A blanked display gives
the wall's processes only one buffer swap per second. The whole wall then
stalls in lockstep, with log lines like
`MPI_Barrier ... took 1.000s`. Turn it off permanently in
`/etc/X11/xorg.conf.d/10-no-dpms.conf`:

```
Section "ServerFlags"
    Option "BlankTime"   "0"
    Option "StandbyTime" "0"
    Option "SuspendTime" "0"
    Option "OffTime"     "0"
EndSection

Section "Extensions"
    Option "DPMS" "Disable"
EndSection
```

Restart X. `DISPLAY=:0 xset q` should then show a screen saver timeout of 0
and no DPMS section.

### 5. Screens, resolutions and xorg.conf

Each display process draws on one X display, given as `display` in its entry
in the configuration file (e.g. `:0`, or `:0.1` for the second X screen).
How you lay out the X screens is up to you:

- **One X screen spanning several monitors** (one GPU, or NVIDIA's
  `BaseMosaic`). One process per node, with several tiles at different `x`/`y`
  offsets within that screen.
- **One X screen per GPU** (`:0.0`, `:0.1`). One process for each X screen,
  each with its own tiles. Each GPU needs its own `Device` and `Screen`
  section in `xorg.conf`, with the right `BusID` (from `nvidia-smi
  --query-gpu=pci.bus_id --format=csv`, written in decimal as `PCI:b:d:f`),
  and `BaseMosaic` must be off. Otherwise the GPUs merge into one screen.

Set the resolution you want with `MetaModes` in each `Screen` section. Check
what each monitor is actually running with `DISPLAY=:0 xrandr`. `xdpyinfo`
reports the size of the whole X screen, not of each monitor, so a monitor
that fell back to a lower mode shows up only in `xrandr`.

If a monitor won't take a mode, add `Option "ModeDebug" "true"` to its
`Device` section. Restart X and read `/var/log/Xorg.0.log`, which then says
why each mode was rejected. For 8K over HDMI 2.1, the usual causes are:
- the TV's extended-signal setting is off (Samsung calls it Input Signal Plus);
- a cable that isn't rated for 48 Gbit/s;
- the wrong input on the TV.

Copy a working node's `xorg.conf` to the others only if they have identical
GPUs in identical slots. Otherwise fix the `BusID`s.

### 6. MPI between the nodes

From the head node, as the wall's account:

```bash
ssh node1 true                          # must not ask for a password
ssh node1 DISPLAY=:0 xdpyinfo | head -3 # must reach node1's X server
mpirun -host localhost,node1,node2 hostname
```

If the `mpirun` test hangs, Open MPI is probably trying a network interface
the nodes can't use between them, e.g. a public interface or a VPN. Name the
right one: `mpirun --mca btl_tcp_if_include eno1 ...`. In the launcher,
that goes in `DISPLAYCLUSTER_MPIARGS`.

## Describing the wall: configuration.json

The configuration file describes the wall's geometry and which process draws
which tiles. It's specific to your wall, so keep it outside the repo, e.g. in
the wall account's home directory as `~/wall.json`.

```json
{
    "dimensions": {
        "numTilesWidth": 2,
        "numTilesHeight": 2,
        "screenWidth": 3840,
        "screenHeight": 2160,
        "mullionWidth": 120,
        "mullionHeight": 120,
        "fullscreen": 1
    },
    "processes": [
        {
            "host": "node1",
            "display": ":0",
            "screens": [
                { "x": 0,    "y": 0, "i": 0, "j": 0 },
                { "x": 3840, "y": 0, "i": 1, "j": 0 }
            ]
        },
        {
            "host": "node2",
            "display": ":0",
            "screens": [
                { "x": 0,    "y": 0, "i": 0, "j": 1 },
                { "x": 3840, "y": 0, "i": 1, "j": 1 }
            ]
        }
    ]
}
```

**`dimensions`** describes the whole wall:
- `numTilesWidth` and `numTilesHeight`: the number of tiles across and down.
- `screenWidth` and `screenHeight`: each tile's size in pixels.
- `mullionWidth` and `mullionHeight`: the gap between neighbouring tiles' active
  areas, in pixels at the tiles' pixel pitch. Content behind a bezel is hidden
  rather than squeezed, so lines stay straight across tiles. Measure the gap
  with a ruler and convert to pixels.
- `fullscreen`: 1 makes each tile window full-screen and borderless; 0 makes
  plain windows, for testing on a desktop.

**`processes`** lists the display processes, one entry for each. Entry *N*
becomes MPI rank *N*+1 (rank 0 is the control process and isn't listed):
- `host`: the machine it runs on. The launcher starts it there over SSH.
- `display`: the X display it draws on. The default is `:0`.
- `screens`: the tiles it draws:
  - `x`, `y`: where the tile's window goes on that X display, in pixels from
    its top-left corner.
  - `i`, `j`: which tile of the wall it is. `i` is the column from the left,
    `j` the row from the top, both from 0.

Every tile `(i, j)` of the grid should appear exactly once. Start from
`config/configuration_stallion.json` (80 tiles on 20 nodes, one X screen
spanning four monitors per node) or `examples/configuration.json` (four small
windows on one machine). `python/xml2json_config.py` converts an old-style
`configuration.xml`.

**A one-machine wall** has a single process, on `localhost`, with a tile for
each monitor. Here are two 4K monitors side by side, as one X screen:

```json
{
    "dimensions": {
        "numTilesWidth": 2, "numTilesHeight": 1,
        "screenWidth": 3840, "screenHeight": 2160,
        "mullionWidth": 60, "mullionHeight": 0,
        "fullscreen": 1
    },
    "processes": [
        {
            "host": "localhost",
            "display": ":0",
            "screens": [
                { "x": 0,    "y": 0, "i": 0, "j": 0 },
                { "x": 3840, "y": 0, "i": 1, "j": 0 }
            ]
        }
    ]
}
```

The control window opens on the same X display. To keep it off the wall,
give the control process a different display, e.g. a VNC desktop, by
starting the launcher with `DISPLAY` set to it.

To try the software before the wall hardware is ready, put every process on
`localhost` with `"fullscreen": 0` and small `screenWidth`/`screenHeight`.
The wall then appears as windows on your desktop.

## The launcher: startdisplaycluster

`scripts/startdisplaycluster` starts a containerized wall. It's the one file a
wall needs outside the image. Copy it somewhere on the wall account's `PATH`
and edit it:

```bash
mkdir -p ~/bin
cp scripts/startdisplaycluster ~/bin/          # or, with no checkout:
apptainer exec displaycluster.sif cat /usr/local/src/DisplayCluster/scripts/startdisplaycluster > ~/bin/startdisplaycluster
chmod +x ~/bin/startdisplaycluster
```

The settings are at the top, each with a comment. You'll usually change:

| Setting | Default | Set it to |
|---|---|---|
| `DISPLAYCLUSTER_SIF` | `~/displaycluster.sif` | where you put the image |
| `DISPLAYCLUSTER_CONFIG` | `~/wall.json` | your configuration file |
| `CONTENT_DIR` | `~/Content` | the wall's working directory, where your media lives |
| `DISPLAYCLUSTER_MEDIA_DIRS` | `content=$CONTENT_DIR` | the folders the web UI's file browser shows |
| `DISPLAYCLUSTER_TIMEOUT` | `10800` | seconds idle before the screensaver starts |
| `DISPLAYCLUSTER_MPIARGS` | `-bind-to none` | add `--mca btl_tcp_if_include <interface>` if MPI picks the wrong network |

Each setting can also be given in the environment, which wins over the file.
For example, `DISPLAYCLUSTER_TIMEOUT=60 startdisplaycluster` is handy while
testing. Any other `DISPLAYCLUSTER_*` variable you add is passed to every
process automatically; see the
[reference](#environment-variable-reference).

When it runs, the launcher:
1. checks that the image and configuration can be read;
2. builds the host list: `localhost` for rank 0, then each process's `host`;
3. changes to `CONTENT_DIR`;
4. prints the `mpirun` command and runs it.

Each rank runs inside the container. `displaycluster.py` sets `DISPLAY` from
the configuration and starts the program. Ctrl-C, or killing the launcher,
stops the whole wall.

## First run

Do these in order. Each one rules out a layer.

1. **The image runs.** On the head node:
   `apptainer exec --nv displaycluster.sif nvidia-smi` lists the GPUs.
2. **One machine, in windows.** Copy `examples/configuration.json` to a test
   file. Set every `host` to `localhost` and every `display` to your
   desktop's (`echo $DISPLAY`). Run:
   ```bash
   DISPLAYCLUSTER_CONFIG=~/test.json startdisplaycluster
   ```
   You should get a control window and four small tile windows.
3. **The wall.** Run `startdisplaycluster` with the real configuration, then
   turn on **View → Show Test Pattern** in the control window. Every tile
   shows a coloured diagonal grid, labelled with its rank, host, display,
   tile coordinates and resolution. The grid's lines should run straight across the whole wall. A
   tile with the wrong label, or lines that jump at a tile's edge, means its
   entry in the configuration is wrong. Turn the test pattern off again in the
   same menu.
4. **Content.** Put an image in `CONTENT_DIR`. Open it from the control
   window's File menu, or from the web UI (below). Drag it across tile
   boundaries and check that lines stay straight across the bezels. If not,
   adjust the mullions. Any image with a grid of straight lines on it works
   as a test pattern.

The log goes to the terminal you started it from. Set
`DISPLAYCLUSTER_LOG_THRESHOLD=2` for more detail. Each line starts with the
rank (`[rank 3] ...`), so you can tell which node said what.

## Using the control window

Rank 0 shows the control window on the head node's screen. It's a scaled-down
picture of the whole wall. Whatever you do in it happens on the wall. Most
people use the [web UI](#remote-control-and-the-web-ui) instead, which does
the same things from any device. While someone else has control through the
API, the control window shows who, and a **Take control** button.

**The File menu**

| Item | What it does |
|---|---|
| Open Content | opens a file in a new window. Images (anything Qt reads: PNG, JPEG, TIFF, ...), SVG, movies (`.mov .avi .mp4 .mkv .mpg .flv .wmv`) and image pyramids (`.pyr`) |
| Open Contents Directory | opens every file in a folder, laid out in a grid; it asks for the grid's columns and rows |
| Clear | closes every window |
| Save State, Load State | save the layout of every window to a `.dcx` file, and restore it |
| Compute Image Pyramid | prepares a very large image for fast display (below) |
| Quit | stops the whole wall |

**The View menu** turns display options on and off: Constrain Aspect Ratio,
Show Window Borders, Show Content Labels, Show Test Pattern, Enable Mullion
Compensation (on by default: content behind a bezel is hidden, so lines stay
straight across tiles), and Show Zoom Context. Its Streaming submenu has settings for pixel
streams. The web UI's options panel has the same settings.

**Windows**, in the picture of the wall:

| To | Do |
|---|---|
| move a window | drag it |
| resize it | drag its bottom-right corner, or use the scroll wheel over it |
| close it | click its top-right corner |
| bring it to the front | click it |
| zoom and pan its content | double-click it to select it (its border changes); then the scroll wheel zooms, dragging pans, and right-dragging zooms. Double-click again to go back to moving and resizing |

The **Contents** list beside the picture lists every open window.

### Very large images: image pyramids

An image up to 4096 pixels on a side is loaded whole. A larger one is shown
from a multi-resolution version, so the wall loads only the detail it needs.
Opening a large image directly works, but is slower and uses more memory.
For an image you'll show more than once, build the pyramid ahead of time:

1. **File → Compute Image Pyramid**, and choose the image. The control window
   stops responding until it's done, which can take minutes for a very large
   image.
2. It writes a folder, `<image>.pyramid/`, and a small file, `<image>.pyr`,
   next to the image.
3. Open the `.pyr` file rather than the image, from the control window, the
   web UI or `DC.py`.

The `.pyr` file holds the pyramid folder's full path. You can rename or move
the `.pyr` file. If you move the folder, edit the path in the `.pyr` file to
match.

## Remote control and the web UI

The control process serves a web UI, and an API for scripts, at
`https://<head node>:1910/ui/` (`http://` without a certificate). It's how
most people use the wall: open, move and resize content, save and load
layouts, all from a laptop, phone or tablet.

Out of the box it's reachable only from the head node itself. To open it to
other machines:

1. **Create a token**, a shared password every client must give:
   ```bash
   mkdir -p ~/.displaycluster && chmod 700 ~/.displaycluster
   python3 -c "import secrets; print(secrets.token_hex(32))" > ~/.displaycluster/api_token
   chmod 600 ~/.displaycluster/api_token
   ```
   With a token, the API listens on all interfaces. Without one, it's
   localhost only.
2. **Add a certificate** for HTTPS. Without one, the token crosses the network
   in the clear. Put the certificate chain in `~/.displaycluster/api_cert.pem`
   and its key in `~/.displaycluster/api_key.pem` (mode 600). A certificate
   from a public CA (e.g. Let's Encrypt, or your institution's IT department)
   for the head node's full DNS name works on every device with no setup. A
   self-signed one works too, but each device then has to be told to trust
   it. [doc/remote-control.md](remote-control.md#https) has the commands for
   both.
3. **Open port 1910** in the head node's firewall:
   ```bash
   sudo firewall-cmd --permanent --add-port=1910/tcp && sudo firewall-cmd --reload   # Rocky
   sudo ufw allow 1910/tcp                                                         # Ubuntu
   ```
   Open 1701 the same way if presenters will use DesktopStreamer.
4. **Restart the wall.** The token and certificate are read only at startup.
   The log should say
   `remote API listening on 0.0.0.0:1910 (https, token required)`.

Then browse to `https://<head node's full name>:1910/ui/` and enter the token.
To give someone access without them typing it, send them
`https://<head node>:1910/ui/#token=<token>`. Anyone with that link can
control the wall, so send it privately.

[doc/remote-control.md](remote-control.md) covers the web UI, phones and
tablets, the browser-tab streaming extension, scripting with `python/DC.py`,
taking turns at control, sensors, and the full API.

## Showing a screen or a browser tab

There are two ways to show something live from a presenter's computer:

| | Browser extension | DesktopStreamer |
|---|---|---|
| Shows | one browser tab | the screen, or a rectangle of it |
| Install | load a folder into Chrome or Edge | build a program on the presenter's computer |
| Runs on | anything with Chrome or Edge 116+ | Linux (X11), macOS, Windows |
| Connects to | port 1910, with the wall's token, over HTTPS if the wall has a certificate | port 1701, no token, unencrypted |
| Sound | no | no |

Use the extension for anything that's in a browser: dashboards, web
visualizations, slides in Google Slides or PowerPoint Online. Use
DesktopStreamer for other programs.

Each stream becomes a window on the wall. It opens when the stream starts and
closes when it stops. You can move and resize it like any other window. While
something is streaming, the screensaver stays off.

### The browser extension

**Getting it to presenters.** The extension isn't in the Chrome Web Store, so
each presenter needs a copy of the repo's `extension/` folder. Zip it and
share it:

```bash
cd DisplayCluster && zip -r displaycluster-extension.zip extension
```

**Installing it**, on the presenter's computer:

1. Unzip it somewhere permanent. Chrome loads it from that folder every time,
   and disables it if the folder moves.
2. Open `chrome://extensions` (in Edge, `edge://extensions`) and turn on
   **Developer mode**.
3. Click **Load unpacked** and choose the `extension` folder.
4. Pin it: the puzzle-piece button in the toolbar, then the pin beside
   **DisplayCluster Streamer**.
5. Open its options (**⋮** → **Options**, or **Set up…** in its popup). Enter:
   - the wall's address: `https://<head node's full name>`, or just the host
     name for a wall without a certificate. Port 1910 is assumed.
   - the wall's token.

   Click **Save**, and allow the permission Chrome asks for. Then **Test
   connection**.

With HTTPS, the browser must trust the wall's certificate. A CA-issued
certificate just works. With a self-signed one, open
`https://<head node>:1910/ui/` in the same browser first and make sure it
loads without a warning. The extension can't click through a warning page.

**Using it:** in the tab to show, click the extension's button, then **Start
streaming**. **Alt+Shift+W** starts and stops it without the popup. Several
tabs can stream at once. The tab keeps streaming in the background and as it
navigates. The picture is sent at the tab's size on your screen, so a bigger
browser window gives a sharper picture on the wall.

[doc/remote-control.md](remote-control.md#streaming-a-browser-tab) covers the
options: frame rate, quality, and largest size.

### DesktopStreamer

DesktopStreamer is a separate Qt program, in `streamer/`. It has its own
CMake project and needs only Qt 6, Boost serialization and libjpeg-turbo: no
MPI, FFmpeg or CUDA. Build it on each presenter's computer.

**Linux (Ubuntu):**
```bash
sudo apt install build-essential cmake pkg-config git qt6-base-dev \
    libboost-serialization-dev libturbojpeg0-dev
cmake -S streamer -B build-streamer
cmake --build build-streamer -j$(nproc)
build-streamer/DesktopStreamer
```

**Linux (Rocky 9):**
```bash
sudo dnf install epel-release
sudo dnf install gcc-c++ cmake pkgconf-pkg-config git qt6-qtbase-devel \
    boost-devel libjpeg-turbo-devel turbojpeg turbojpeg-devel
```
then the same `cmake` commands as for Ubuntu.

DesktopStreamer captures the screen through X11. In a Wayland session the
picture may come back black. At the login screen, choose an "on Xorg"
session instead.

**macOS:**
```bash
brew install cmake qt boost jpeg-turbo
cmake -S streamer -B build-streamer -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build-streamer -j8
open build-streamer/DesktopStreamer.app
```
The first time it shares, macOS asks for Screen Recording permission, under
System Settings → Privacy & Security. Grant it and restart DesktopStreamer.
If frames come back black and it never asked, find it in that list, remove
it, and try again.

**Windows:** see "Building DesktopStreamer" in the
[README](../README.md#building-desktopstreamer). It's a vcpkg build, like the
main program, but with no MPI, CUDA or FFmpeg.

**Using it:**

1. **Hostname:** the wall's head node.
2. **Stream name:** the name the wall's window gets. Two presenters must use
   different names.
3. **X, Y, Width, Height:** the part of the screen to send. The default is
   the whole primary screen. **Options → Show Rectangle** shows the area as a
   rectangle you can drag and resize.
4. **Max frame rate:** frames per second, 24 by default. Lower it if the
   network is slow.
5. Click **Share**. The window appears on the wall. **End Sharing** removes
   it.

It always connects to port 1701, so leave `DISPLAYCLUSTER_NETWORKPORT` at its
default. It sends JPEG frames with no encryption: don't show anything
confidential over a network you don't trust.

## Updating

**Container:**
```bash
cd DisplayCluster && git pull && ./build-sif.sh
```
Stop the wall, copy the new `displaycluster.sif` into place if it lives
elsewhere, and start the wall again. Check the new launcher in `scripts/` for
new settings now and then, but your edited copy keeps working.

**Native:** `git pull`, rebuild, `cmake --install` on every node (or the
shared prefix), and restart the wall.

To confirm you're running what you think you are, check the version in the
control window, which is the git commit it was built from. With
`DISPLAYCLUSTER_LOG_THRESHOLD=2`, every rank also logs it at startup.

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Log shows `MPI_Barrier ... took 1.000s` over and over; the wall updates once a second | a display has blanked (DPMS) and its swaps are throttled | [turn off DPMS](#4-turn-off-screen-blanking-and-dpms) and restart X |
| X.service fails: `Permission denied` on `/dev/dri`, "no screens found" | a login manager (GDM etc.) has the GPUs | [disable it](#2-run-x-as-a-service-not-a-desktop) and reboot |
| A display process says it can't open display `:0.1` | the second GPU isn't its own X screen | separate `Device`/`Screen` sections per GPU, `BaseMosaic` off |
| A monitor runs at a lower resolution than the others | the mode was rejected: cable, TV input setting, or port | `xrandr` to see it; `ModeDebug` in `xorg.conf` to see why |
| Display processes can't open the display: "Authorization required" | the wall account isn't allowed on X | [`xhost +si:localuser:<user>`](#3-let-the-walls-account-draw-on-it) |
| `mpirun` hangs at start | MPI chose a network the nodes can't use | `--mca btl_tcp_if_include <interface>` in `DISPLAYCLUSTER_MPIARGS` |
| `mpirun` asks for passwords | no passwordless SSH to the nodes | set up SSH keys for the wall account |
| MPI version mismatch errors from the container | host Open MPI isn't 4.1.x | install Open MPI 4.1 on the hosts |
| After a rebuild, the wall still runs the old code | the launcher points at a different `.sif` | check `DISPLAYCLUSTER_SIF`, and the version in the control window |
| The screensaver keeps clearing your content | `DISPLAYCLUSTER_TIMEOUT` is short (the default is 5 s) | set it in the launcher |
| Web UI unreachable from other machines | no token (so localhost only), firewall, or not on the network (e.g. VPN required) | check the "listening on" line in the log; `curl -k https://<head>:1910/ui/` from the client |
| Web UI says "That token didn't work"; console shows 401 on `/config` | the token doesn't match the one the wall loaded | copy it again; restart the wall if the token file changed |
| Browser warns the connection isn't private | self-signed certificate, or the URL's name isn't on the certificate | use the full name on the certificate; use a CA-issued certificate |
| Extension: red **!**, "couldn't reach the wall" | wrong address, port 1910 blocked, or the browser doesn't trust the certificate | open `https://<head>:1910/ui/` in that browser; fix whatever it shows |
| Extension: "the wall refused the stream" | wrong token | re-enter it in the extension's options |
| DesktopStreamer: nothing appears on the wall | port 1701 blocked, or the wrong host name | `nc -vz <head> 1701` from the presenter's machine |
| DesktopStreamer: the wall shows black | Wayland on Linux; no Screen Recording permission on macOS | use an Xorg session; grant the permission and restart it |
| Log says the remote UI files weren't found | `DISPLAYCLUSTER_DIR` doesn't point at the install prefix | set it to the prefix (`/usr/local` in the container) |

## Environment variable reference

The launcher passes every one of these to every process.

| Variable | Default | What it does |
|---|---|---|
| `DISPLAYCLUSTER_SIF` | (launcher) `~/displaycluster.sif` | the container image |
| `DISPLAYCLUSTER_CONFIG` | `$DISPLAYCLUSTER_DIR/configuration.json` | the wall's configuration file |
| `DISPLAYCLUSTER_DIR` | required; launcher sets `/usr/local` | install prefix: where `data/` and `ui/` are |
| `DISPLAYCLUSTER_INSTALL` | `/usr/local` | where `bin/displaycluster` is |
| `DISPLAYCLUSTER_EXEC` | `displaycluster` | the program's name under `bin/` |
| `DISPLAYCLUSTER_MPIARGS` | (launcher) `-bind-to none` | extra `mpirun` arguments |
| `DISPLAYCLUSTER_TIMEOUT` | `5` | seconds idle before the screensaver |
| `DISPLAYCLUSTER_SCREENSAVER_IMAGE` | none | the screensaver's image; a relative path is under `DISPLAYCLUSTER_DIR` |
| `DISPLAYCLUSTER_LOG_THRESHOLD` | `4` | log detail: 1 debug, 2 info, 3 warnings, 4 errors |
| `DISPLAYCLUSTER_NETWORKPORT` | `1701` | DesktopStreamer's port; DesktopStreamer always uses 1701, so leave it |
| `DISPLAYCLUSTER_API_PORT` | `1910` | the web UI and API's port |
| `DISPLAYCLUSTER_API_TOKEN` | `~/.displaycluster/api_token` | the API token |
| `DISPLAYCLUSTER_API_BIND` | `0.0.0.0` with a token, else `127.0.0.1` | the API's listening address |
| `DISPLAYCLUSTER_API_CERT`, `_KEY` | `~/.displaycluster/api_cert.pem`, `api_key.pem` | HTTPS certificate and key |
| `DISPLAYCLUSTER_STATE_DIR` | `~/.displaycluster/states` | where saved layouts go |
| `DISPLAYCLUSTER_MEDIA_DIRS` | home directory | folders the web UI browses, `name=path:name=path` |
| `DISPLAYCLUSTER_UI_DIR` | `$DISPLAYCLUSTER_DIR/ui` | the web UI's files |
| `DISPLAYCLUSTER_SENSORS` | `0` | sensors offered in the web UI |
| `DISPLAYCLUSTER_DRAG_SYNC_MS` | `0` | while dragging in the control window, update the wall this often (ms); 0 moves it only on release |
| `DISPLAYCLUSTER_FRAME_STATS` | unset | set to log each rank's frame rate |

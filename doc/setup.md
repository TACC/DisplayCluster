# Setting up DisplayCluster: from GitHub to a working wall

This guide takes you from nothing to a running wall. It assumes you know your
way around a Linux shell, but nothing about DisplayCluster. For the short
version, see [quickstart.md](quickstart.md).

- [How DisplayCluster works](#how-displaycluster-works)
- [Choosing a build](#choosing-a-build)
- [What every wall needs](#what-every-wall-needs)
- [Getting the code](#getting-the-code)
- [Container build](#container-build)
- [Native build](#native-build)
- [Preparing the display nodes](#preparing-the-display-nodes)
- [Describing the wall: configuration.json](#describing-the-wall-configurationjson)
- [The DisplayCluster directory](#the-displaycluster-directory)
- [The launcher: startdisplaycluster](#the-launcher-startdisplaycluster)
- [First run](#first-run)
- [Using the control window](#using-the-control-window)
- [Running it unattended](#running-it-unattended)
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
| `build-sif.sh` | builds the container image, into a DisplayCluster directory with `-r` |
| `scripts/startdisplaycluster` | the launcher you copy and edit for your wall |
| `scripts/displaycluster.sh`, `displaycluster.py` | run each rank. The launcher calls them; you don't edit them. |
| `scripts/install-displaycluster` | installs an image, and the scripts that came with it, in a [DisplayCluster directory](#the-displaycluster-directory) |
| `scripts/killdc` | stops the wall and cleans up after it on every host in its configuration, when Ctrl-C or killing `mpirun` leaves ranks behind (`killdc -n` shows what it would stop) |
| `scripts/cyclestates` | shows a list of saved layouts in turn ([remote-control.md](remote-control.md#cycling-through-states-cyclestates)) |
| `scripts/check-dcx`, `localize-dcx` | check and repair state files, and make self-contained copies of them ([content.md](content.md)) |
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
image, plus 2.5 GB in `/tmp` (or `$TMPDIR`) while it runs.

### 2. Build

From the top of the checkout (not from inside `docker/`), naming the
[DisplayCluster directory](#the-displaycluster-directory) to build into:

```bash
./build-sif.sh -r /shared/DisplayCluster
```

(or set `DISPLAYCLUSTER_ROOT` instead of `-r`). This runs these steps and
stops at the first one that fails:

1. `docker build -f docker/Dockerfile -t displaycluster .` compiles
   DisplayCluster inside an Ubuntu 22.04 image with Qt 6, Boost, Open MPI,
   FFmpeg and the CUDA driver API. The image is based on NVIDIA's
   `nvidia/opengl` image. It copies your checkout, including any uncommitted
   changes, so what you build is what's in the directory.
2. `docker save` writes the image to an archive in a temporary directory.
3. `apptainer build` converts that archive to an image in the directory's
   `images/`, named `displaycluster-<date>-<commit>.sif` (`-modified` after
   the commit if you'd changed files without committing them).
4. `install-displaycluster` makes it the image the wall runs from its next
   start, installs the scripts and `DC.py` that came with it, and deletes all
   but the three newest images (`-k` changes how many).

Without a directory, it writes `displaycluster.sif` in the checkout instead,
for you to install with `install-displaycluster`.

It uses `docker save` and apptainer's `docker-archive:` rather than
`apptainer build ... docker-daemon:displaycluster:latest`. On rattler-dev,
`docker-daemon:` reached a different Docker daemon than the `docker` command
did, and silently built an old image into the `.sif` however often it was
rebuilt. An archive written by the same `docker` that built the image can't
do that.

The image is based on NVIDIA's `nvidia/opengl` image, for NVIDIA GPUs. The
`FROM` line in `docker/Dockerfile` can name a plain Ubuntu 22.04 instead, but
it then draws with Mesa, which can struggle at a wall's resolutions. Its Open
MPI is Ubuntu's 4.1.2, which is why the hosts need a matching 4.1; the
Dockerfile can install another version instead, untested.

It cleans up after itself: the archive is deleted however the script ends,
and so are the images earlier builds left behind. Those are labelled as
DisplayCluster's, so it leaves other people's Docker images alone. Images
from builds before October 2026 aren't labelled. To clear them too,
`docker image prune` removes every untagged image on the machine, so check
with whoever else uses Docker there first.

The first build downloads and installs a lot and takes 15–30 minutes. Later
builds reuse Docker's cache and take a few minutes. The result is about
900 MB.

Check that it worked:

```bash
ls -l /shared/DisplayCluster/images            # displaycluster.sif points at today's
apptainer exec /shared/DisplayCluster/images/displaycluster.sif ls /usr/local/bin/displaycluster
```

### 3. If the build machine can't see the wall's directory

Build without `-r`, copy `displaycluster.sif` to the wall's head node, and
install it there with `install-displaycluster`, as in [The DisplayCluster
directory](#the-displaycluster-directory). Every machine in the wall must be
able to read the directory at the same path.

### What's inside the image

| Path in the image | What it is |
|---|---|
| `/usr/local/bin/displaycluster` | the program |
| `/usr/local/ui`, `/usr/local/data`, `/usr/local/examples` | web UI, images, examples |
| `/usr/local/bin/check-dcx`, `localize-dcx` | state file tools; they run on the host too ([content.md](content.md#running-the-scripts)) |
| `/usr/local/bin/cyclestates`, `/usr/local/python/DC.py` | shows saved layouts in turn ([remote-control.md](remote-control.md#cycling-through-states-cyclestates)), and the Python client it uses |
| `/usr/local/src/DisplayCluster` | the source it was built from, including `scripts/` (`install-displaycluster` copies the launcher and the other scripts from here) |

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

Whether Ubuntu's own FFmpeg libraries can decode on the GPU depends on the
release: `ffmpeg -hwaccels` should list `cuda`. If it doesn't, a prebuilt
FFmpeg with NVDEC is the fallback, as on [Windows](#windows).

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
There's no CUDA, so CUDA is off by default and movies decode on the CPU,
using every core. A 4K movie keeps most of an 8-core Mac busy.

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

What's been tested on Windows is a wall in windows on one machine's desktop,
launched with `mpiexec -n <1 + display processes>`. The launcher scripts and
the display-node preparation below are for Linux.

#### Toolchain

- **Visual Studio 2022** (Community or higher, or just the Build Tools for
  Visual Studio 2022) with the **Desktop development with C++** workload. It
  provides MSVC, the Windows SDK, and the **x64 Native Tools Command Prompt
  for VS 2022**. Build from that prompt, not a plain terminal: `cl.exe` needs
  the environment (`INCLUDE`, `LIB`, `PATH`) it sets up, which `vcvars64.bat`
  (under `VC\Auxiliary\Build` in the install) also does. In a `.bat` that
  calls `vcvars64.bat` and logs to a file, send vcvars' own output to a
  different file: a process it starts keeps that file open, so every later
  `>> same.log` fails and its command silently doesn't run.
- **Git for Windows.**
- **CMake 3.16+**, from the workload's **C++ CMake tools for Windows**
  component or installed on its own. If `cmake` isn't found in the Native
  Tools prompt, add that component in the Visual Studio Installer.
- **[vcpkg](https://github.com/microsoft/vcpkg)**, cloned to a path with **no
  spaces** (`C:\vcpkg`, not under `C:\Users\Some User\...`): several of its
  ports run MSYS2 build scripts that break on spaces. Clone DisplayCluster to
  a space-free path too, such as `C:\src\DisplayCluster`.

  ```
  git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
  C:\vcpkg\bootstrap-vcpkg.bat
  ```

#### Installed by hand

- **MS-MPI:** both the SDK (`msmpisdk.msi`) and the runtime (`msmpisetup.exe`)
  from Microsoft. CMake finds it through the `MSMPI_INC` and `MSMPI_LIB64`
  variables the SDK's installer sets, so open a new prompt after installing
  it.
- **The NVIDIA driver**, whatever's current for the card.
- **The NVIDIA CUDA Toolkit** (optional), for hardware movie decode (NVDEC):
  the driver API's headers (`cuda.h`, `cudaGL.h`) and `cuda.lib`. Its
  installer sets `CUDA_PATH`, which the build searches. Without it, configure
  with `-DDISPLAYCLUSTER_CUDA=OFF`, and movies decode in software.
- **FFmpeg, prebuilt, with NVDEC.** vcpkg's FFmpeg port fails to build on
  Windows (a malformed `compat/windows/makedef` call making `libavcodec`'s
  export file), so use a prebuilt one from
  [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds/releases): a
  `win64-*-shared` variant, such as `ffmpeg-master-latest-win64-gpl-shared.zip`
  (`-shared` matters: it has the `.lib` and `.dll` files). Extract it, for
  example to `C:\ffmpeg`, and check that `C:\ffmpeg\bin\ffmpeg.exe -hide_banner
  -hwaccels` lists `cuda`.

Everything else (Qt 6 with `qtsvg`, Boost, libjpeg-turbo and OpenSSL 3) is in
`vcpkg.json`, and vcpkg installs it during CMake's configure.

#### Configure and build

From the x64 Native Tools Command Prompt:

```
set FFMPEG_DIR=C:\ffmpeg
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

(add `-DDISPLAYCLUSTER_CUDA=OFF` to the first line without the CUDA Toolkit).

The first configure builds the whole vcpkg set from source, about 94 packages,
mostly Boost and Qt: 25 minutes on a fast many-core machine, well over an hour
on others. vcpkg keeps what it built (`%LOCALAPPDATA%\vcpkg\archives`), so
later configures, even in a fresh `build\` or another project using the same
packages, are quick.

Things to check in CMake's output:
- `remote API: HTTPS supported (OpenSSL 3.x)`;
- `Found MPI: TRUE` (if not, the prompt is older than the MS-MPI SDK);
- `Building without CUDA: movies decode in software` appears only with CUDA
  off.

To turn CUDA on later, install the Toolkit, open a new prompt (so `CUDA_PATH`
is set), and configure again with `-DDISPLAYCLUSTER_CUDA=ON`; with vcpkg's
packages already built, that takes seconds. As of CUDA 13, `cuda.lib` is a
small static loader that opens `nvcuda.dll` when CUDA is first used, so
`dumpbin /dependents` doesn't list `nvcuda.dll`; that's expected.

#### Running it on Windows

The build copies the DLLs it needs (Qt, Boost, OpenSSL, FFmpeg, and the
libraries Qt's plugins need) next to `displaycluster.exe`, so `C:\ffmpeg\bin`
needn't be on `PATH`; `msmpi.dll` comes from the MS-MPI runtime. Qt's plugin
folders aren't copied, so set `QT_PLUGIN_PATH`:

```
set DISPLAYCLUSTER_DIR=C:\src\DisplayCluster
set DISPLAYCLUSTER_CONFIG=C:\src\DisplayCluster\configuration.json
set DISPLAYCLUSTER_TIMEOUT=3600
set QT_PLUGIN_PATH=C:\src\DisplayCluster\build\vcpkg_installed\x64-windows\Qt6\plugins
mpiexec -n 2 C:\src\DisplayCluster\build\Release\displaycluster.exe
```

`DISPLAYCLUSTER_TIMEOUT` is the screensaver's idle time in seconds; without
it, the screensaver starts after 5 and clears test content. For a first test,
a configuration with one 800×600 tile in a window, with `mpiexec -n 2` (the
control window and one display process):

```json
{
    "dimensions": {
        "numTilesWidth": 1, "numTilesHeight": 1,
        "screenWidth": 800, "screenHeight": 600,
        "mullionWidth": 0, "mullionHeight": 0,
        "fullscreen": 0
    },
    "processes": [
        { "host": "localhost", "screens": [ {"x": 0, "y": 0, "i": 0, "j": 0} ] }
    ]
}
```

The web UI is then at <http://localhost:1910/ui/>; the API listens only on
`127.0.0.1` until the wall has a token. Its file browser starts in your home
folder. To offer other folders, set `DISPLAYCLUSTER_MEDIA_DIRS`, with `;`
between them on Windows: `set
DISPLAYCLUSTER_MEDIA_DIRS=media=D:\wall-media;C:\src\DisplayCluster\examples`.

#### Hardware movie decode, and laptops with two GPUs

With CUDA on, NVDEC decodes each movie on the NVIDIA GPU, and CUDA copies the
frames straight into the tile's OpenGL textures. That last step works only
when the NVIDIA GPU also draws the window. Laptops with hybrid graphics (an
integrated Intel or AMD GPU and an NVIDIA one) give new programs the
integrated GPU, and the display process then logs:

```
Movie::render: can't share GL textures with CUDA (...; GL renderer AMD Radeon(TM) 610M), so hardware-decoded frames will be copied through host memory
```

Movies still play, but every frame goes through system memory to the other
GPU. To fix it, give `displaycluster.exe` the NVIDIA GPU: **Settings → System
→ Display → Graphics → Add an app → Desktop app**, choose
`build\Release\displaycluster.exe`, then **Options → High performance** (or,
in the NVIDIA Control Panel, **Manage 3D settings → Program Settings →
High-performance NVIDIA processor**). A build at a different path needs the
setting again. Desktops whose monitors are on the NVIDIA card don't need any
of this.

To check that a movie is decoding on the GPU, run `nvidia-smi dmon -s u`: the
`dec` column should be above 0. `nvidia-smi` should list the display process
with type `C+G` (CUDA and graphics on that GPU), and the "can't share GL
textures" line shouldn't appear. Task Manager's GPU tab shows the decoder's
load as "Video Decode".

On a node with several GPUs, each with its own X screen and its own
DisplayCluster process (as on Stallion), each process decodes on the GPU
behind its own windows, and logs which:

```
movies decode on CUDA device 1, NVIDIA RTX PRO 6000 Blackwell Server Edition (0001:C5:00.0), the GPU showing this process's windows
```

There, `nvidia-smi dmon -s u` should show `dec` load on every GPU whose
screens show a movie.

### Configure, build, install (Linux)

Build outside the source tree. The container build copies the whole checkout,
and a `build/` inside it gets dragged along by accident.

```bash
cmake -S . -B ../dc-build -DCMAKE_INSTALL_PREFIX=/shared/DisplayCluster
cmake --build ../dc-build -j$(nproc)
cmake --install ../dc-build
```

The prefix is the [DisplayCluster directory](#the-displaycluster-directory)
(with `DISPLAYCLUSTER_ROOT` set, it's the default).

Things to check in CMake's output:
- `remote API: HTTPS supported (OpenSSL 3.x)`. If it says "HTTP only",
  install the OpenSSL development package and configure again.
- `Building without CUDA` appears only if you turned CUDA off.

The install puts `bin/displaycluster`, the launcher and the other scripts in
`bin/`, and `ui/`, `data/`, `examples/`, `python/` and `doc/` beside it. The
first time, it also makes `Content/` and writes an example `wall.json`; after
that it leaves both alone. Install to a shared filesystem, or the same path on
every display node. (`-DDISPLAYCLUSTER_WALL_DIRECTORY=OFF` leaves out
`Content/` and `wall.json`, for a prefix that isn't a DisplayCluster
directory.)

The default build type is `RelWithDebInfo`: optimized, but with debugging
symbols, so a crash gives a useful backtrace.

### Running a native build

`bin/startdisplaycluster` runs the native build installed beside it when the
directory has no image (`images/displaycluster.sif`), with `DISPLAYCLUSTER_DIR`
set to the directory. Everything else about it is as for the container. To run
it by hand instead:

```bash
export DISPLAYCLUSTER_DIR=/shared/DisplayCluster
export DISPLAYCLUSTER_CONFIG=$DISPLAYCLUSTER_DIR/wall.json
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
which tiles. It's specific to your wall, so keep it outside the repo, as
`wall.json` in the [DisplayCluster directory](#the-displaycluster-directory).

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
`config/configuration_stallion.json` (TACC's Stallion: eighteen 8K TVs, 6
across and 3 down, on 9 nodes, each with two GPUs run as separate X screens
`:0` and `:0.1`, one process each) or `examples/configuration.json` (four small
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

### Several screens on a node: one process or one per GPU

A node driving more than one screen can run a single process with a tile for
each screen, or a process per screen. Which is right depends on how many
GPUs drive the screens. The two set up X differently, and handle content
differently.

**One GPU driving several screens: one process.** Make a single X screen
that spans the monitors, and give the process a tile for each, at its pixel
offset in that X screen, as in the example above. The process's windows share
one OpenGL context, so each image is loaded once, and each movie decoded once,
however many of its screens show it. The tiles are drawn one after another,
then swapped one after another.

**A GPU per screen: one process per GPU.** Run each GPU as its own X screen
(`:0`, `:0.1`, ...), and give each its own process with one tile. This is
Stallion's setup: two GPUs per node, two processes per node. Don't make one X
screen span several GPUs (with Xinerama or NVIDIA Mosaic) to run one process.
The process's shared OpenGL context assumes a single GPU, so drawing would be
slow at best.

With a process per GPU, nothing is shared between the processes. Each one
loads every image and decodes every movie that its tile shows, by itself.
- **Each process decodes the whole movie, not just the part on its tile.** A
  movie spanning both of a node's screens is decoded twice on that node, once
  on each GPU. One spanning Stallion's whole wall is decoded 18 times, once
  per process.
- **Each process decodes on its own screen's GPU.** It decodes with that
  GPU's NVDEC decoder and hands frames straight to the OpenGL textures on the
  same GPU. Its log names the GPU at the first movie:
  `movies decode on CUDA device 1, ... (0001:C5:00.0)`.
- **Older builds decoded every movie on CUDA device 0.** Before this was
  fixed, a node's second process decoded on the first GPU too. Its frames
  then crossed to its own GPU through system memory, and the second GPU's
  decoder sat idle.
- **Check that the decoding is spread out.** With a movie on both of a node's
  screens, run `nvidia-smi dmon -s u` there: the `dec` column should show load
  on both GPUs.

Heavy movie use is limited by the decoders: count how many processes show
each movie, not how many movies there are.

## The DisplayCluster directory

A wall's files all live in one directory, on a filesystem that every node of
the wall sees at the same path (a shared home directory works; on a single
machine, any local folder does):

```
DisplayCluster/                   DISPLAYCLUSTER_ROOT
  bin/                            startdisplaycluster, killdc, install-displaycluster,
                                  cyclestates, check-dcx, localize-dcx
  images/displaycluster-<built>-<commit>.sif    each image installed
  images/displaycluster.sif       -> the one the wall runs
  python/                         DC.py
  wall.json                       the wall's layout
  site.sh                         optional: this site's settings
  Content/                        media and saved states; the wall's working directory
```

The scripts in `bin/` find the directory from where they are, as the folder
above `bin/`, so they need no editing and a symlink to one (in `~/bin`, say)
works too. Setting `DISPLAYCLUSTER_ROOT` picks another directory, for example
a test install beside the real one.

`Content/` holds both the media and the states saved from the web UI or
`DC.py`, so a project can keep its files and its layouts in one folder, such as
`Content/talks/morning/`. Only the API token and the HTTPS certificate and key
stay in the wall account's `~/.displaycluster/`, since they're private.

**Filling it.** `build-sif.sh -r` builds an image straight into the directory
and installs it ([Container build](#container-build)), and a native build's
`cmake --install` fills it too ([Native build](#native-build)). To install an
image built elsewhere, use `install-displaycluster`, which is in every image:

```bash
apptainer exec displaycluster.sif cat /usr/local/src/DisplayCluster/scripts/install-displaycluster |
    bash -s -- -r /shared/DisplayCluster displaycluster.sif
```

It makes the directory if it isn't there, and:
- copies the image into `images/`, named for when it was built and the commit
  it was built from, and points `images/displaycluster.sif` at it, so the wall
  runs it from its next start;
- installs the scripts and `DC.py` that came with it in `bin/` and `python/`;
- the first time only, writes an example `wall.json` and makes `Content/`.
  It never changes `wall.json`, `site.sh` or `Content/` once they're there;
- keeps the three newest images (`-k` changes how many) and deletes older ones.

Once it's installed, `bin/install-displaycluster new.sif` does the same for
the next image, and finds the directory itself.

Then edit `wall.json` ([above](#describing-the-wall-configurationjson)), and
put the directory's `bin/` on the wall account's `PATH`.

## The launcher: startdisplaycluster

`bin/startdisplaycluster` starts a containerized wall. It runs the image in
`images/`, with `wall.json`, and starts the wall in `Content/`, which is also
where the web UI browses and saves states.

Its settings are environment variables, each with a comment and a default at
the top of the script. Rather than edit the script, put the ones you change in
`site.sh` in the DisplayCluster directory, which it reads first, so an update
that replaces the script keeps them:

```bash
# DisplayCluster/site.sh
DISPLAYCLUSTER_SENSORS=6
DISPLAYCLUSTER_MPIARGS="-bind-to none --mca btl_tcp_if_include enp0s25"
```

You'll usually set only a few:

| Setting | Default | Set it to |
|---|---|---|
| `DISPLAYCLUSTER_SIF` | `images/displaycluster.sif` | another image |
| `DISPLAYCLUSTER_CONFIG` | `wall.json` | another configuration file |
| `CONTENT_DIR` | `Content` | the wall's working directory, where your media lives |
| `DISPLAYCLUSTER_MEDIA_DIRS` | `content=$CONTENT_DIR` | the folders the web UI's file browser shows |
| `DISPLAYCLUSTER_STATE_DIR` | `$CONTENT_DIR` | where states are saved and loaded |
| `DISPLAYCLUSTER_TIMEOUT` | `10800` | seconds idle before the screensaver starts |
| `DISPLAYCLUSTER_SENSORS` | `0` | the number of [sensors](remote-control.md#sensors); the web UI shows no sensor controls at 0 |
| `DISPLAYCLUSTER_MPIARGS` | `-bind-to none` | add `--mca btl_tcp_if_include <interface>` if MPI picks the wrong network |

The paths are in the DisplayCluster directory. A setting given in the
environment wins over both `site.sh` and the default, which is handy while
testing: `DISPLAYCLUSTER_TIMEOUT=60 startdisplaycluster`. Any other `DISPLAYCLUSTER_*`
variable you add is passed to every process automatically; see the
[reference](#environment-variable-reference). So is `APPTAINER_BINDPATH`,
which makes folders outside your home directory visible in the container
(see [content.md](content.md#where-content-can-live)). The launcher adds the
DisplayCluster directory to it when that's outside your home directory.

When it runs, the launcher:
1. reads `site.sh`, then checks that the image, the configuration and the
   content folder are there;
2. warns if states are still in `~/.displaycluster/states`, where earlier
   versions kept them, since the wall no longer shows them. Move them into
   `Content/`, or keep them where they are with
   `DISPLAYCLUSTER_STATE_DIR=$HOME/.displaycluster/states` in `site.sh`;
3. builds the host list: `localhost` for rank 0, then each process's `host`;
4. changes to `CONTENT_DIR`;
5. prints the `mpirun` command and runs it.

Each rank runs inside the container. `displaycluster.py` sets `DISPLAY` from
the configuration and starts the program. Ctrl-C, or killing the launcher,
stops the whole wall. If ranks are left behind (after a crash, say), `killdc`
stops whatever's left on every host in `wall.json`; `killdc -n` shows what it
would stop.

## First run

Do these in order. Each one rules out a layer.

1. **The image runs.** On the head node:
   `apptainer exec --nv /shared/DisplayCluster/images/displaycluster.sif nvidia-smi`
   lists the GPUs.
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
4. **Content.** Put an image in the directory's `Content/`. Open it from the control
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
| Save State, Load State | save the layout of every window to a `.dcx` file, and restore it. The file names content by path; see [content.md](content.md) |
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
match. A relative path there is taken relative to the `.pyr` file's own
folder, so `"photo.pyramid" 40000 30000` keeps working wherever the two are
moved together.

### Where content lives, and moving layouts

[content.md](content.md) covers where content can be put so every node can
read it, how state (`.dcx`) files name their content, what breaks when files
move, and the two scripts for that: `localize-dcx`, which gathers a layout and
its content into one movable folder, and `check-dcx`, which finds and repairs
broken layouts.

## Running it unattended

Started from a terminal, the wall stops when that terminal closes, and its log
goes only to that terminal. For a wall that should run all the time, use one
of the following.

The control process always needs an X display for its control window:
- the head node's own screen, if it has one;
- a display node's screen, if the head node is also a display node;
- a VNC desktop on the head node.

Set `DISPLAY` to that display wherever you start it.

### In tmux

The simplest way to keep it running after you log out:

```bash
tmux new -d -s wall 'DISPLAY=:1 startdisplaycluster 2>&1 | tee -a ~/displaycluster.log'
tmux attach -t wall        # watch it; Ctrl-B then D to leave it running
tmux kill-session -t wall  # stop the wall
```

The log is in `~/displaycluster.log` as well. It keeps growing, so empty it
now and then.

### As a systemd service

To start the wall at boot and restart it if it crashes, run it as a service.
On the head node, as root, create `/etc/systemd/system/displaycluster.service`:

```ini
[Unit]
Description=DisplayCluster wall
Wants=network-online.target
After=network-online.target X.service

[Service]
User=vislab
Environment=DISPLAY=:0
# systemd's PATH is minimal: add where mpirun is (on Rocky, /usr/lib64/openmpi/bin)
Environment=PATH=/usr/lib64/openmpi/bin:/usr/local/bin:/usr/bin:/bin
ExecStart=/home/vislab/DisplayCluster/bin/startdisplaycluster
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Change `User`, `DISPLAY` and the launcher's path to suit. Then:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now displaycluster
sudo systemctl restart displaycluster      # e.g. after updating the image
journalctl -u displaycluster -f            # the log
```

`systemctl stop` stops every rank. **File → Quit** in the control window ends
the service too, without a restart, since that isn't a failure. Start it again
with `systemctl start`.

When the display nodes boot at the same time, their X servers may not be up
when the wall starts. The wall then fails, and systemd tries again 10 seconds
later until they are.

### On a schedule

To have the wall on during the day and off at night, start and stop it with
the system's scheduler. Stopping means ending `mpirun` (`mpiexec` on Windows),
which stops every process of the wall.

If systemd runs the wall, use one or the other: schedule `systemctl start` and
`systemctl stop` (as root) rather than the commands below. If you only want a
dark wall at night, you can leave it running and put it to sleep
(`sleepWall()` in `DC.py`, or **Sleep** in the web UI).

The same schedule can start [cyclestates](remote-control.md#cycling-through-states-cyclestates),
which shows a list of saved layouts in turn.

**Linux: cron.** As the wall's account, run `crontab -e` and add:

```
PATH=/home/vislab/DisplayCluster/bin:/usr/lib64/openmpi/bin:/usr/local/bin:/usr/bin:/bin
DISPLAY=:0
DISPLAYCLUSTER_API_HTTPS=1

# weekdays: the wall on at 7:00, off at 20:00
0 7 * * 1-5   pgrep -x mpirun -u $LOGNAME > /dev/null || startdisplaycluster >> $HOME/displaycluster.log 2>&1
0 20 * * 1-5  pkill -TERM -x mpirun -u $LOGNAME

# cycle through states while it's on
1 7 * * 1-5   pgrep -f '[c]yclestates' > /dev/null || cyclestates -H wallhost -f $HOME/states.txt >> $HOME/cyclestates.log 2>&1
0 20 * * 1-5  pkill -f '[c]yclestates'
```

- Cron's environment is minimal, so set `PATH` to include the launcher,
  `mpirun`, `apptainer` and `python3`, and `DISPLAY` as in
  [Running it unattended](#running-it-unattended). A cron file doesn't expand
  variables in these settings, so write them out in full.
- The `pgrep` keeps a second wall from starting if one is already running.
- The brackets in `[c]yclestates` stop the pattern from matching the shell
  cron runs the line in.
- `-H wallhost` and `DISPLAYCLUSTER_API_HTTPS=1` apply when the wall uses
  HTTPS. The host must be the name on its certificate, not `localhost`.
- Cron won't bring the wall back after a crash or a reboot during the day. For
  that, make the first line `*/10 7-19 * * 1-5`, which checks every 10 minutes.
  It also restarts a wall someone quit on purpose.
- The logs keep growing, so empty them now and then.

**macOS: launchd.** A wall needs a logged-in session to show windows, so use a
LaunchAgent. Create `~/Library/LaunchAgents/org.tacc.displaycluster.start.plist`,
changing the paths:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>org.tacc.displaycluster.start</string>
    <key>ProgramArguments</key>
    <array>
        <string>/Users/you/DisplayCluster/scripts/run-macos.sh</string>
    </array>
    <key>EnvironmentVariables</key>
    <dict>
        <key>PATH</key>
        <string>/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin</string>
    </dict>
    <key>StartCalendarInterval</key>
    <dict>
        <key>Hour</key><integer>7</integer>
        <key>Minute</key><integer>0</integer>
    </dict>
    <key>StandardOutPath</key>
    <string>/Users/you/Library/Logs/displaycluster.log</string>
    <key>StandardErrorPath</key>
    <string>/Users/you/Library/Logs/displaycluster.log</string>
</dict>
</plist>
```

For the stop, make a second file, `org.tacc.displaycluster.stop.plist`, with its
own `Label`, an `Hour` of 20, no logs or `EnvironmentVariables`, and these
`ProgramArguments`:

```xml
    <key>ProgramArguments</key>
    <array>
        <string>/usr/bin/pkill</string>
        <string>-TERM</string>
        <string>-x</string>
        <string>mpirun</string>
    </array>
```

Then load both:

```bash
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/org.tacc.displaycluster.start.plist
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/org.tacc.displaycluster.stop.plist
```

To limit a job to certain days, make `StartCalendarInterval` an array of those
dictionaries, each with a `Weekday` (0 is Sunday). The account must be logged
in. If the Mac is asleep at the set time, the job runs when it wakes; to keep
it awake, use **System Settings → Energy** or `pmset`. To remove a job,
use `launchctl bootout` with the same arguments.

**Windows: Task Scheduler.** Put the commands from
[Running it on Windows](#running-it-on-windows) in a batch file, such as
`C:\src\start-wall.bat`:

```bat
set DISPLAYCLUSTER_DIR=C:\src\DisplayCluster
set DISPLAYCLUSTER_CONFIG=C:\src\DisplayCluster\configuration.json
set QT_PLUGIN_PATH=C:\src\DisplayCluster\build\vcpkg_installed\x64-windows\Qt6\plugins
mpiexec -n 2 C:\src\DisplayCluster\build\Release\displaycluster.exe >> C:\src\displaycluster.log 2>&1
```

Then create the two tasks, from a prompt as the wall's account:

```bat
schtasks /Create /TN "DisplayCluster start" /SC WEEKLY /D MON,TUE,WED,THU,FRI /ST 07:00 /IT /TR "C:\src\start-wall.bat"
schtasks /Create /TN "DisplayCluster stop" /SC WEEKLY /D MON,TUE,WED,THU,FRI /ST 20:00 /IT /TR "taskkill /IM mpiexec.exe /T /F"
```

`/IT` runs a task only while the account is logged in, which the wall needs to
show its windows. To run `cyclestates` too, add a task that runs
`python C:\src\DisplayCluster\scripts\cyclestates -f C:\src\states.txt`, and end it the
same way by its window or process. To remove a task, use
`schtasks /Delete /TN "DisplayCluster start"`.

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

Certificates expire, often after 90 days for Let's Encrypt. Since the wall
reads its certificate only at startup, a renewed one needs copying into place
and a restart of the wall.
[doc/remote-control.md](remote-control.md#renewing-the-certificate) has a
certbot hook that does the copying.

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
    boost-devel boost-serialization \
    libjpeg-turbo-devel turbojpeg turbojpeg-devel
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

**From the top of the checkout:** configuring with
`-DDISPLAYCLUSTER_CLIENT_ONLY=ON` builds DesktopStreamer the same way, without
the wall, and `cmake --install` installs it with the client tools: `python/`
(`DC.py` and the examples), `bin/cyclestates`, and the Chrome extension in
`extension/`, ready to load unpacked:

```bash
cmake -S . -B build-client -DDISPLAYCLUSTER_CLIENT_ONLY=ON -DCMAKE_INSTALL_PREFIX=$HOME/DisplayCluster
cmake --build build-client -j8 && cmake --install build-client
```

**Windows:** the same toolchain as the [main program's](#windows) (Visual
Studio 2022, the x64 Native Tools prompt, vcpkg on a space-free path), but
only vcpkg is needed: no MS-MPI, CUDA or FFmpeg. `streamer/vcpkg.json` lists
the packages, and they install during the configure:

```
cd streamer
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
set QT_PLUGIN_PATH=%CD%\build\vcpkg_installed\x64-windows\Qt6\plugins
build\Release\DesktopStreamer.exe
```

As with the main program, `QT_PLUGIN_PATH` is needed because vcpkg copies the
DLLs next to the program but not Qt's plugin folders.

**Handing it to presenters.** A build runs on the machine it was built on,
using the Qt it was built against. To give it to someone without a build
setup, package it with its Qt first:

- **Windows:** from the "x64 Native Tools Command Prompt for VS 2022", in
  `streamer`, after building:
  ```
  mkdir dist\DesktopStreamer
  copy build\Release\DesktopStreamer.exe dist\DesktopStreamer\
  copy build\Release\*.dll dist\DesktopStreamer\
  build\vcpkg_installed\x64-windows\tools\Qt6\bin\windeployqt.exe --release --no-translations dist\DesktopStreamer\DesktopStreamer.exe
  ```
  `windeployqt` adds Qt's plugins, and Microsoft's `vc_redist.x64.exe`,
  which installs the Visual C++ runtime on a computer that lacks it. Zip
  `dist\DesktopStreamer` and hand it out; the presenter unzips it and runs
  `DesktopStreamer.exe`, with no `QT_PLUGIN_PATH` needed. The folder is
  about 110 MB before zipping.
- **macOS:** after building, run
  ```bash
  streamer/package-macos.sh build-streamer/DesktopStreamer.app
  ```
  It makes the app self-contained and writes `build-streamer/DesktopStreamer.dmg`
  (about 40 MB) to hand out. Plain `macdeployqt` isn't enough with Homebrew's
  Qt: the app it makes still loads Homebrew's Qt as well as its own, and
  doesn't start on a Mac without Homebrew. The script fixes that, and checks
  that nothing in the app refers outside it. The app isn't signed with a
  developer ID, so the first time, the presenter opens it with right-click →
  Open (or System Settings → Privacy & Security → Open Anyway). It runs on
  the kind of Mac it was built on; an Intel build also runs on Apple silicon,
  under Rosetta.
- **Linux:** `cmake --install build-streamer --prefix /usr/local` (or another
  prefix) installs `bin/DesktopStreamer`. It uses the system's Qt, so install
  it on machines running the same distribution, with the packages listed
  above.

Build the copy you hand out from the same revision as the wall, so the two
agree on how frames are sent.

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

## A wall account's files: checklist

Everything a containerized wall needs is in the
[DisplayCluster directory](#the-displaycluster-directory), apart from the
wall account's private files in `~/.displaycluster/`. Use this list to set up
a new wall, a second wall, or a replacement account. On an existing wall, use
it to check what's where. Paths without `~` are in the DisplayCluster
directory.

| What | Default place | Set by | Notes |
|---|---|---|---|
| the image | `images/displaycluster.sif` | `DISPLAYCLUSTER_SIF` | a link to the one the launcher runs, among the installed `displaycluster-*.sif`; see below |
| the launcher, `killdc`, `install-displaycluster` | `bin/` | | installed with the image, unedited |
| the site's settings | `site.sh` | | optional; read by the launcher and `killdc` |
| the wall's layout | `wall.json` | `DISPLAYCLUSTER_CONFIG` | [configuration.json](#describing-the-wall-configurationjson) |
| content | `Content/` | `CONTENT_DIR`, `DISPLAYCLUSTER_MEDIA_DIRS` | readable from every node; see [content.md](content.md) |
| saved states | `Content/` | `DISPLAYCLUSTER_STATE_DIR` | beside the media they show |
| `DC.py` | `python/` | | for `cyclestates` and your own scripts |
| state file tools | `bin/check-dcx`, `bin/localize-dcx` | | see [content.md](content.md#running-the-scripts) |
| API token | `~/.displaycluster/api_token` | `DISPLAYCLUSTER_API_TOKEN` | mode 600; see [remote-control.md](remote-control.md#the-token) |
| HTTPS certificate and key | `~/.displaycluster/api_cert.pem`, `api_key.pem` | `DISPLAYCLUSTER_API_CERT`, `_KEY` | an RSA certificate, key mode 600; read only at startup |
| starting and stopping | a systemd unit, or [a schedule](#on-a-schedule) | | [Running it unattended](#running-it-unattended) |

**The launcher runs `images/displaycluster.sif`, whatever else is there.**
`install-displaycluster` points it at an image; `ls -l images` shows which.
Copying an image into `images/` by hand doesn't change what runs.

**After installing, confirm what's running.** Check the version, which is
the git commit the image was built from, in any of these places:
- the control window;
- each process's startup log line (`version ...`), with
  `DISPLAYCLUSTER_LOG_THRESHOLD=2`;
- `GET /config`:
  ```bash
  curl -s -H "Authorization: Bearer $(cat ~/.displaycluster/api_token)" https://wallhost:1910/config
  ```

To check that a copied image arrived intact, compare `sha256sum
displaycluster.sif` with the same on the build machine.

## Updating

**Container:**
```bash
cd DisplayCluster && git pull && ./build-sif.sh -r /shared/DisplayCluster
```
The new image and its scripts are installed as it finishes; the wall needn't
be stopped for that. Restart the wall to run it (`killdc`, then
`startdisplaycluster`), and [confirm the version](#a-wall-accounts-files-checklist).
The site's settings are in `site.sh`, so the new scripts keep them.

**Going back** to an earlier image: install it again from `images/`, and
restart the wall:

```bash
install-displaycluster /shared/DisplayCluster/images/displaycluster-20261101-1934-35831b9.sif
```

**Moving from a launcher you edited.** Earlier versions had you edit a copy
of the launcher, usually `~/bin/startdisplaycluster`, and kept the image,
`wall.json`, `Content` and states in the home directory. To move: set up the
DisplayCluster directory, move the image into `images/`, `wall.json` and
`Content/` into it, and the states from `~/.displaycluster/states` into
`Content/`. Copy any settings you'd changed at the top of the old launcher
into `site.sh`, then delete the old launcher so it isn't run by mistake.

**Native:** `git pull`, rebuild, `cmake --install` into the directory, and
restart the wall.

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
| Chrome refuses the web UI with `NET::ERR_CERT_AUTHORITY_INVALID`, but Safari and `curl` accept it | an ECDSA certificate on a chain whose root Chrome doesn't trust (InCommon's ECC chain, for one) | get it reissued with an RSA key; see [HTTPS](remote-control.md#https) |
| Extension: red **!**, "couldn't reach the wall" | wrong address, port 1910 blocked, or the browser doesn't trust the certificate | open `https://<head>:1910/ui/` in that browser; fix whatever it shows |
| Extension: "the wall refused the stream" | wrong token | re-enter it in the extension's options |
| DesktopStreamer: nothing appears on the wall | port 1701 blocked, or the wrong host name | `nc -vz <head> 1701` from the presenter's machine |
| DesktopStreamer: the wall shows black | Wayland on Linux; no Screen Recording permission on macOS | use an Xorg session; grant the permission and restart it |
| Log says the remote UI files weren't found | `DISPLAYCLUSTER_DIR` doesn't point at the install prefix | set it to the prefix (`/usr/local` in the container) |
| Log: `can't share GL textures with CUDA`, naming a non-NVIDIA (or Mesa) GL renderer | the tiles are drawn by a GPU CUDA can't share with: a laptop's integrated GPU, or a VNC/Mesa desktop | Windows: give `displaycluster.exe` the NVIDIA GPU in Settings → System → Display → Graphics; Linux: run on the NVIDIA X server |
| Windows: "`<name>.dll` was not found" when starting | `msmpi.dll`: the MS-MPI runtime isn't installed; anything else: an exe built before the DLL-copy step | install `msmpisetup.exe`; rebuild |
| Windows: CMake can't find MPI | the prompt was opened before the MS-MPI SDK was installed | open a new prompt and configure again |
| No sensor controls in the web UI | `DISPLAYCLUSTER_SENSORS` is 0, the default | set it to the number of sensors in the launcher, and restart the wall ([Sensors](remote-control.md#sensors)) |
| Windows: a batch build stops after `vcvars64.bat` but exits 0 | vcvars' output and later commands share one log file, which a process vcvars starts keeps open | send vcvars' output to its own file |

## Environment variable reference

The launcher passes every one of these to every process.

| Variable | Default | What it does |
|---|---|---|
| `DISPLAYCLUSTER_ROOT` | (launcher, `killdc`) the folder above their `bin/` | the [DisplayCluster directory](#the-displaycluster-directory) |
| `DISPLAYCLUSTER_SIF` | (launcher) `$DISPLAYCLUSTER_ROOT/images/displaycluster.sif` | the container image |
| `DISPLAYCLUSTER_CONFIG` | `$DISPLAYCLUSTER_DIR/configuration.json` | the wall's configuration file |
| `DISPLAYCLUSTER_DIR` | required; launcher sets `/usr/local` | install prefix: where `data/` and `ui/` are |
| `DISPLAYCLUSTER_INSTALL` | `/usr/local` | where `bin/displaycluster` is |
| `DISPLAYCLUSTER_EXEC` | `displaycluster` | the program's name under `bin/` |
| `DISPLAYCLUSTER_MPIARGS` | (launcher) `-bind-to none` | extra `mpirun` arguments |
| `DISPLAYCLUSTER_SCRIPTS` | (launcher) `/usr/local/src/DisplayCluster/scripts` | where, inside the image, the launcher finds `displaycluster.sh` and `.py` |
| `DISPLAYCLUSTER_TIMEOUT` | `5` | seconds idle before the screensaver |
| `DISPLAYCLUSTER_SCREENSAVER_IMAGE` | none | the screensaver's image; a relative path is under `DISPLAYCLUSTER_DIR` |
| `DISPLAYCLUSTER_LOG_THRESHOLD` | `4` | log detail: 1 debug, 2 info, 3 warnings, 4 errors |
| `DISPLAYCLUSTER_NETWORKPORT` | `1701` | DesktopStreamer's port; DesktopStreamer always uses 1701, so leave it |
| `DISPLAYCLUSTER_API_PORT` | `1910` | the web UI and API's port |
| `DISPLAYCLUSTER_API_TOKEN` | `~/.displaycluster/api_token` | the API token |
| `DISPLAYCLUSTER_API_BIND` | `0.0.0.0` with a token, else `127.0.0.1` | the API's listening address |
| `DISPLAYCLUSTER_API_CERT`, `_KEY` | `~/.displaycluster/api_cert.pem`, `api_key.pem` | HTTPS certificate and key |
| `DISPLAYCLUSTER_STATE_DIR` | (launcher) `$DISPLAYCLUSTER_ROOT/Content`; otherwise `~/.displaycluster/states` | where saved layouts go |
| `DISPLAYCLUSTER_MEDIA_DIRS` | (launcher) `content=$DISPLAYCLUSTER_ROOT/Content`; otherwise the home directory | folders the web UI browses, `name=path:name=path` (`;` between them on Windows) |
| `DISPLAYCLUSTER_UI_DIR` | `$DISPLAYCLUSTER_DIR/ui` | the web UI's files |
| `DISPLAYCLUSTER_SENSORS` | `0` | sensors offered in the web UI; at 0 it shows no sensor controls. Read at startup |
| `DISPLAYCLUSTER_DRAG_SYNC_MS` | `0` | while dragging in the control window, update the wall this often (ms); 0 moves it only on release |
| `DISPLAYCLUSTER_FRAME_STATS` | unset | set to log each rank's frame rate |

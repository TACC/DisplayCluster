# DisplayCluster

There are two ways to build and run DisplayCluster: natively, if you can
install the platform's dependencies directly, or via an apptainer/Docker
container, if you'd rather not — useful for reproducible deployment across
cluster nodes without managing native dependencies on every worker. Both
produce the same application; pick whichever fits your situation.

## Native build

### Windows 11

#### Toolchain

- Visual Studio 2022 (Community or higher) with the **"Desktop development
  with C++"** workload — provides MSVC, the Windows SDK, and the "x64 Native
  Tools Command Prompt for VS 2022". **Builds must be run from this specific
  prompt**, not a plain terminal — `cl.exe` needs its environment
  (`INCLUDE`/`LIB`/`PATH`) initialized, which only happens there (or after
  manually running `vcvarsall.bat x64`).
- Git for Windows
- CMake 3.16+ (bundled with VS2022, or install standalone)

#### Package manager

- [vcpkg](https://github.com/microsoft/vcpkg), cloned to a **path with no
  spaces** (e.g. `C:\vcpkg`, not under `C:\Users\Some User\...`). Several
  vcpkg ports drive MSYS2-based build scripts internally, which break on
  spaces in paths.
- Clone this project to a space-free path too, for the same reason (e.g.
  `C:\src\DisplayCluster`).

```
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

#### Installed manually (not vcpkg-managed)

- **MS-MPI** — install both the SDK (`msmpisdk.msi`) and the runtime
  (`msmpisetup.exe`) from Microsoft. CMake's `FindMPI` locates it
  automatically via the `MSMPI_INC`/`MSMPI_LIB64` environment variables the
  SDK installer sets.
- **NVIDIA GPU driver** — whatever's current for the card.
- **NVIDIA CUDA Toolkit** — needed for the driver-API headers (`cuda.h`,
  `cudaGL.h`) and import library (`cuda.lib`); the installer sets
  `CUDA_PATH`, which `CMakeLists.txt` searches automatically.
- **FFmpeg with NVDEC, prebuilt** — building FFmpeg from source via vcpkg's
  MSYS2-driven Windows port hit an unresolved build failure (a malformed
  `compat/windows/makedef` invocation generating `libavcodec`'s DLL export
  file). Sidestep it entirely with a prebuilt binary from
  [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds/releases) — grab
  a `win64-*-shared` variant (e.g. `ffmpeg-master-latest-win64-gpl-shared.zip`,
  the `-shared` suffix matters, it's what provides `.lib`/`.dll` instead of a
  static executable), extract it anywhere (e.g. `C:\ffmpeg`), and verify NVDEC
  support before wiring it in:
  ```
  C:\ffmpeg\bin\ffmpeg.exe -hide_banner -hwaccels
  ```
  You want `cuda` in that list.

#### vcpkg-managed dependencies

Everything else — Qt6 (`qtbase` with `widgets`/`network`/`opengl` features,
plus `qtsvg`), Boost (`serialization`, `date-time`, `iostreams`, `algorithm`,
`tokenizer`, `smart-ptr`), and `libjpeg-turbo` — is declared in `vcpkg.json`
and installs automatically during CMake configure. No manual step needed.

#### Configure and build

From the **x64 Native Tools Command Prompt for VS 2022**:

```
set FFMPEG_DIR=C:\ffmpeg
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

The first configure builds the full vcpkg dependency set from source
(mostly Qt6) and can take well over an hour; vcpkg caches build artifacts
machine-wide afterward (`%LOCALAPPDATA%\vcpkg\archives`), so subsequent
configures — even in a fresh `build\` directory, or from another project
using the same package/version/feature/triplet combination — are fast.

#### Running

vcpkg copies the runtime DLLs (Qt, Boost, etc.) next to `displaycluster.exe`
automatically, but **not** Qt's plugin subdirectories (platform integration,
image format loaders), so `QT_PLUGIN_PATH` needs to be set explicitly:

```
set DISPLAYCLUSTER_DIR=C:\src\DisplayCluster
set DISPLAYCLUSTER_CONFIG=C:\src\DisplayCluster\configuration.json
set DISPLAYCLUSTER_TIMEOUT=3600
set QT_PLUGIN_PATH=C:\src\DisplayCluster\build\vcpkg_installed\x64-windows\Qt6\plugins
mpiexec -n 2 C:\src\DisplayCluster\build\Release\displaycluster.exe
```

`DISPLAYCLUSTER_TIMEOUT` (seconds) controls the screensaver idle timeout;
set it high while testing, or the bouncing-logo screensaver will kick in and
clear your test content every 5 seconds (the default).

`configuration.json` is machine-specific and not checked in — see
`config/configuration_stallion.json` or `examples/configuration.json` for a
template; the simplest single-tile version is:

```json
{
    "dimensions": {
        "numTilesWidth": 1,
        "numTilesHeight": 1,
        "screenWidth": 800,
        "screenHeight": 600,
        "mullionWidth": 0,
        "mullionHeight": 0,
        "fullscreen": 0
    },
    "processes": [
        { "host": "localhost", "screens": [ {"x": 0, "y": 0, "i": 0, "j": 0} ] }
    ]
}
```

### Ubuntu

Package names below are current as of a recent Ubuntu release; adjust via
`apt search` if a name has changed on your version.

```bash
sudo apt install build-essential cmake pkg-config git \
    qt6-base-dev qt6-svg-dev \
    libboost-serialization-dev libboost-date-time-dev libboost-iostreams-dev \
    libjpeg-turbo8-dev \
    libopenmpi-dev openmpi-bin \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev \
    libgl1-mesa-dev libglu1-mesa-dev
```

Plus, outside `apt`:

- **NVIDIA GPU driver** (`ubuntu-drivers autoinstall`, or install manually)
- **NVIDIA CUDA Toolkit** — for `cuda.h`/`cudaGL.h`; the driver package alone
  provides `libcuda.so` but not the headers

Note: Ubuntu's stock `libavcodec-dev` may or may not have NVDEC/CUDA hwaccel
compiled in, depending on release — confirm with `ffmpeg -hwaccels` (want
`cuda` in the list) before relying on it; if it's missing, a prebuilt
NVDEC-enabled FFmpeg (same approach as the Windows path above) is the
fallback.

`nlohmann::json` is vendored in the repo (`src/json.hpp`) — no separate
package needed.

#### Configure and build

Build out-of-tree, e.g. as a sibling of the repo rather than inside it —
the Dockerfile COPYs the whole repo directory into the image, and an
in-tree `build/` (even though `.gitignore`/`.dockerignore` both exclude
it from tracking/COPY) is easy to end up dragging along by accident:

```bash
cmake -S . -B ../build
cmake --build ../build -j$(nproc)
```

#### Running

```bash
export DISPLAYCLUSTER_DIR=/path/to/DisplayCluster
export DISPLAYCLUSTER_CONFIG=/path/to/DisplayCluster/configuration.json
export DISPLAYCLUSTER_TIMEOUT=3600
mpirun -np 2 ./build/displaycluster
```

Same `configuration.json` and `DISPLAYCLUSTER_TIMEOUT` notes as the Windows
section above apply.

## Building DesktopStreamer

`streamer/` holds a separate, standalone build for the client-side
desktop-sharing tool (`DesktopStreamer`, plus the `dcStream` library it's
built alongside) — meant to run on a presenter's own machine, not a cluster
node. It has its own `CMakeLists.txt` and doesn't use the root project's
build at all: no MPI, CUDA, or FFmpeg required, since none of that code path
is used here. It connects over TCP to port 1701 on whichever host is running
MPI rank 0 of a running DisplayCluster process; the moment it starts sending
frames for a URI, a window for that URI appears on the wall automatically.

### Windows 11

Same toolchain as the main app's Windows section above (Visual Studio 2022
with the C++ workload, built from the "x64 Native Tools Command Prompt for
VS 2022", vcpkg cloned to a space-free path) — but only vcpkg is needed here,
none of the manually-installed pieces (MS-MPI, CUDA, FFmpeg). Dependencies
are declared in `streamer/vcpkg.json` (Qt6 `widgets`/`network` features,
`boost-serialization`, `libjpeg-turbo`) and install automatically:

```
cd streamer
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

Running it needs the same `QT_PLUGIN_PATH` workaround as the main app (vcpkg
copies the DLLs next to the exe but not Qt's plugin subdirectories):

```
set QT_PLUGIN_PATH=C:\src\DisplayCluster\streamer\build\vcpkg_installed\x64-windows\Qt6\plugins
streamer\build\Release\DesktopStreamer.exe
```

### Linux

Only Qt6 (`Core`/`Gui`/`Widgets`/`Network`/`Concurrent`), Boost
(`serialization`), and libjpeg-turbo are needed — a much smaller dependency
set than the main app.

**Ubuntu:**

```bash
sudo apt install build-essential cmake pkg-config git \
    qt6-base-dev \
    libboost-serialization-dev \
    libjpeg-turbo8-dev
```

**Rocky/RHEL 9:** Qt6 comes from EPEL, not the base repos. Also note
`libjpeg-turbo-devel` only provides the CMake config and headers — the
actual `libturbojpeg.so` it points to ships in a separate `turbojpeg`
package, and CMake will fail at configure time with a "file does not exist"
error on the imported target if it's missing:

```bash
sudo dnf install epel-release
sudo dnf install gcc-c++ cmake pkgconf-pkg-config git \
    qt6-qtbase-devel \
    boost-devel boost-serialization \
    libjpeg-turbo-devel turbojpeg turbojpeg-devel
```

**Configure, build, and run** (same on both):

```bash
cmake -S streamer -B build-streamer
cmake --build build-streamer -j$(nproc)
build-streamer/DesktopStreamer
```

### macOS

Untested on real hardware — this is best-effort based on how the equivalent
Homebrew packages are laid out, not a verified build like the Windows/Linux
paths above; expect to adjust something.

```bash
brew install cmake qt@6 boost jpeg-turbo
```

`qt@6` is keg-only (Homebrew won't symlink it into the general prefix, to
avoid clashing with a `qt@5` install), so CMake needs to be told where to
find it explicitly:

```bash
cmake -S streamer -B build-streamer -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build-streamer -j$(sysctl -n hw.ncpu)
open build-streamer/DesktopStreamer.app
```

This builds a real `.app` bundle (`streamer/CMakeLists.txt` sets
`MACOSX_BUNDLE` under `if(APPLE)`), rather than a bare executable — matters
for screen sharing specifically, since macOS ties the Screen Recording
permission prompt to the bundle's identity. The first launch will prompt for
Screen Recording access under System Settings → Privacy & Security; if it
doesn't prompt and frames just come back black, check whether it's listed
there already (denied) and needs to be removed/re-added.

## Container build (apptainer)

Built on top of the NVIDIA OpenGL image
(`nvidia/opengl:1.0-glvnd-devel-ubuntu22.04), primarily for systems with
NVIDIA graphics cards. The `FROM` line in `docker/Dockerfile` can be changed
to a plain Ubuntu 22.04 base instead, but rendering then falls back to Mesa,
which can struggle at high display resolutions.

In the following, *host* refers to the machine outside the container, and
*worker* to a machine running the containerized GUI and display nodes.

Clone the repo somewhere — we'll refer to this as the source directory.
To build the DC image, run this from the source directory itself (not from
inside `docker/` — the Dockerfile `COPY`s the checkout it lives in, rather
than cloning a fresh copy from GitHub, so the build context has to be the
repo root):

```bash
docker build -f docker/Dockerfile -t displaycluster .
docker save displaycluster:latest -o /tmp/displaycluster.tar
apptainer build displaycluster.sif docker-archive:/tmp/displaycluster.tar
```

This creates **displaycluster.sif** with the worker-side bits installed in
the `/usr/local` area of the container.

Use `docker save` + `docker-archive:`, not
`apptainer build ... docker-daemon:displaycluster:latest` directly — on at
least one multi-user setup (TACC's rattler-dev), `apptainer build`'s
`docker-daemon:` transport connected to a different Docker daemon/socket
than the `docker` CLI itself was using, so it kept baking a stale,
previously-built image into the .sif no matter how many times the image was
rebuilt or `apptainer cache clean` was run — completely silent, no error,
just an old binary. `docker save` guarantees you're archiving the exact
image the `docker` CLI just built (same daemon by construction), and
`docker-archive:` reads directly from that file with no daemon connection
involved at all, sidestepping the whole class of bug.

### Host-side code

To run DisplayCluster, you'll need to install a few bits onto the host
system. In the source directory, make a `build` subdirectory, and in there,
run:

```bash
cmake .. -DINSTALL_CLIENT=On [-DCMAKE_INSTALL_PREFIX={host-installdir, default to /usr/local}]
make install
```

This installs the startup command in `{host-installdir}/bin`, the python
package in `{host-installdir}/python`, and some example code in
`{host-installdir}/examples`.

Now copy the sif file into the host install directory:

```bash
mkdir {host-installdir}/sif
cp {source directory}/docker/displaycluster.sif {host-installdir}/sif
```

### Starting DisplayCluster

Start DisplayCluster using the `startdisplaycluster` command in
`{host-installdir}/bin`. It requires some environment setup; a shell script
like this works well:

```bash
#! /bin/bash

## Location of configuration.json ... installed on the client system in {worker-installdir}/examples
export DISPLAYCLUSTER_HOME={worker-installdir}
export DISPLAYCLUSTER_DIR=${DISPLAYCLUSTER_HOME}   # not sure if both HOME and DIR are needed
export DISPLAYCLUSTER_CONFIG=${DISPLAYCLUSTER_HOME}/examples/configuration.json

## tell it where to get the sif file
export DISPLAYCLUSTER_SIF=${DISPLAYCLUSTER_HOME}/sif/displaycluster.sif

export DISPLAYCLUSTER_API_PORT=1910               # remote-control API; see "Remote control API"
export DISPLAYCLUSTER_TIMEOUT=3600                 # screensaver timeout in seconds
export DISPLAYCLUSTER_EXEC=/displaycluster

cd {content directory}
startdisplaycluster
```

This also installs the python package in `{worker-installdir}/python` and
some example code in `{worker-installdir}/examples`. In
`{worker-installdir}/examples`, run:

```bash
export PYTHONPATH={worker-installdir}/python
python3 ./run_script.py
```

### MPI

To run apptainer containers under MPI, the version of MPI on the hosts
*outside* the container must match the version of MPI *inside* the
container. `docker/Dockerfile` installs OpenMPI via the Ubuntu 22.04
package repos (currently 4.1.2). So the one requirement of the host
environment (other than having apptainer installed) is a matching OpenMPI
version. Alternatively, `docker/Dockerfile` can be modified to install a
different version that matches the host's — your mileage may vary if you
choose this alternative.

Separately: DisplayCluster requires an MPI build with `MPI_THREAD_MULTIPLE`
support — its render processes issue MPI calls from more than one thread
concurrently (a per-movie decoder thread alongside the main thread), and it
requests that thread level explicitly at startup and fails fast with a
clear error if the MPI implementation doesn't grant it. Ubuntu 22.04's
packaged OpenMPI supports this; if you swap in a different MPI build,
confirm it does too.

### Installation and runtime

Create a root directory for DisplayCluster in a shared filesystem. Add a
`configuration.json` file to that directory as described above. Place
**displaycluster.sif** there too, along with **examples/startdisplaycluster**
— this python script is where various environment variables are defaulted,
and where the actual `mpirun` call is made.

Note: in some cases (like Rattler here at TACC) OpenMPI doesn't seem to find
the correct interface for MPI to use. This is hard-coded in the
`mpirunCommand` string built near the end of `startdisplaycluster` — you may
need to change it.

## Remote control API

The master process (rank 0) serves an HTTP/JSON API for controlling the wall
remotely, and a web UI for it at `http://wallhost:1910/ui/`; `python/DC.py`
is a Python client. Coordinates are in tile units — `(0, 0)` is the wall's
top-left corner and `(tilesWide, tilesHigh)` its bottom-right — and windows
are addressed by name: by default a window's name is its file's path, with
`#2`, `#3`, ... appended if that file is already open, or you can choose one
when opening it.

### Control

Anyone with the token can look; only one client at a time can change the
wall. A client takes control with `POST /control {"label": "Greg's laptop"}`,
which returns a `lease`; every change then carries it in an `X-DC-Lease`
header. A change without it gets `423`, naming who has control. Control
lapses after 30 seconds without a request carrying the lease — use
`POST /control/heartbeat` to keep it while idle, or keep an `/events` stream
open with `?lease=`. `{"force": true}` takes control from whoever has it.

While a remote client has control, the control window is read-only (it keeps
showing the wall) with a banner and a **Take control** button, and the wall's
idle timer is paused — the controller decides when the wall sleeps. When
control ends, the idle timer restarts. `DC.py` handles all of this: it takes
control the first time it changes something and gives it back when it's done.

### Requests

| Request | Body | Does |
|---|---|---|
| `GET /status` | | `{asleep, idleTimeout, controller}` |
| `GET /events` | | server-sent events: the wall's status, windows and options, now and whenever they change |
| `POST /control` | `{label?, force?}` | take control; returns `{lease, id, label, timeout}` |
| `POST /control/heartbeat`, `DELETE /control` | | keep, or give up, control |
| `GET /config` | | wall size in tiles and pixels, screen and mullion sizes, version |
| `GET /windows` | | every window, back to front |
| `POST /windows` | `{uri, name?, x?, y?, w?, h?}` | open a file |
| `POST /windows/directory` | `{root, dir, cols?, rows?}` | open a folder's files tiled across the wall |
| `DELETE /windows` | | close everything |
| `GET /windows/{name}` | | one window |
| `PATCH /windows/{name}` | any of `{x, y, w, h, hidden, front, zoom, centerX, centerY, name, filled}` | move, resize, hide, raise, zoom, rename; `filled: true` fills the wall (as big as fits, centered, in front) remembering where the window was, `false` puts it back |
| `DELETE /windows/{name}` | | close a window |
| `GET /options`, `PATCH /options` | any of the View menu's options, as booleans | display options |
| `GET /state` | | saved state files, newest first |
| `POST /state/load`, `POST /state/save` | `{file}` | state files, relative to the state directory |
| `POST /sleep`, `POST /wake` | | start or end the screensaver (waking needs no control) |
| `GET /media` | | the media roots |
| `GET /media/{root}` | `?dir=&sort=name\|modified\|size&order=asc\|desc&offset=&limit=` | a page of a directory: folders, then files the wall can open |

`POST`, `PATCH` and `DELETE` requests to `/windows...`, `/options`,
`/state/load` and `/sleep` need control. Names go in the URL
percent-encoded (`/` as `%2F`, `#` as `%23`). Errors come back with a 4xx
status and `{"error": "..."}`.

Changes wake the wall from its screensaver; reads don't, so something that's
only watching doesn't keep it awake. While it's asleep, `/windows` reports the
layout waking will bring back.

### Configuration

- `DISPLAYCLUSTER_API_PORT` — port (default 1910)
- `DISPLAYCLUSTER_API_TOKEN` — a shared secret every request must send as
  `Authorization: Bearer <token>` (or, for `/events`, which browsers can't
  send headers with, as `?access_token=`). If unset, it's read from
  `~/.displaycluster/api_token` if that exists. Generate one with
  `python3 -c "import secrets; print(secrets.token_hex(32))"`. The web UI's
  own files load without it; the page asks for it.
- `DISPLAYCLUSTER_API_BIND` — address to listen on. Defaults to all
  interfaces when a token is configured, and to `127.0.0.1` (reachable only
  from the master's own host, e.g. over an SSH tunnel) when not.
- `DISPLAYCLUSTER_STATE_DIR` — where state files are loaded from and saved
  to (default `~/.displaycluster/states`)
- `DISPLAYCLUSTER_MEDIA_DIRS` — the directories `/media` browses, separated
  like `PATH` (`:`, or `;` on Windows), each optionally named:
  `content=/data/content:/scratch/movies`. An unnamed one is named after its
  last path component. Symlinks are followed only into another root.
  Default: your home directory.
- `DISPLAYCLUSTER_UI_DIR` — where the web UI's files are (default:
  `<install prefix>/ui`)

The token is sent in the clear over plain HTTP; across an untrusted network,
use an SSH tunnel or a TLS-terminating reverse proxy.

```bash
LEASE=$(curl -s -H "Authorization: Bearer $TOKEN" -X POST http://wallhost:1910/control \
        -d '{"label": "curl"}' | python3 -c "import json,sys; print(json.load(sys.stdin)['lease'])")
curl -H "Authorization: Bearer $TOKEN" -H "X-DC-Lease: $LEASE" -X POST http://wallhost:1910/windows \
     -d '{"uri": "/data/images/mars.jpg", "name": "mars", "x": 0, "y": 0, "w": 2, "h": 2}'
```

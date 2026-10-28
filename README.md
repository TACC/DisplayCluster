# DisplayCluster

There are two ways to build and run DisplayCluster: natively, if you can
install the platform's dependencies directly, or via an apptainer/Docker
container, if you'd rather not — useful for reproducible deployment across
cluster nodes without managing native dependencies on every worker. Both
produce the same application; pick whichever fits your situation.

**New to DisplayCluster?** [doc/setup.md](doc/setup.md) walks through
everything from cloning the repo to a working wall: both builds, the
dependencies, preparing the display nodes, the configuration file, the
launcher, remote control, and troubleshooting.

## Native build

### Windows 11

#### Toolchain

- Visual Studio 2022 (Community or higher, or just the Build Tools for
  Visual Studio 2022) with the **"Desktop development with C++"** workload —
  provides MSVC, the Windows SDK, and the "x64 Native Tools Command Prompt for
  VS 2022". **Builds must be run from this specific prompt**, not a plain
  terminal — `cl.exe` needs its environment (`INCLUDE`/`LIB`/`PATH`)
  initialized, which only happens there (or after running `vcvars64.bat`,
  under `VC\Auxiliary\Build` in the Visual Studio or Build Tools install).
  In a `.bat` that calls `vcvars64.bat` and logs to a file, send vcvars'
  own output to a *different* file: a process it starts keeps that file open
  afterward, so every later `>> same.log` fails and its command silently
  doesn't run.
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
  SDK installer sets — open a new prompt after installing the SDK, since
  one that was already open won't have them.
- **NVIDIA GPU driver** — whatever's current for the card.
- **NVIDIA CUDA Toolkit** (optional) — needed for NVDEC hardware movie
  decode: the driver-API headers (`cuda.h`, `cudaGL.h`) and import library
  (`cuda.lib`); the installer sets `CUDA_PATH`, which `CMakeLists.txt`
  searches automatically. Without it, configure with
  `-DDISPLAYCLUSTER_CUDA=OFF` and movies decode in software.
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
`tokenizer`, `smart-ptr`), `libjpeg-turbo`, and OpenSSL 3 (HTTPS for the
remote-control API) — is declared in `vcpkg.json` and installs automatically
during CMake configure. No manual step needed.

#### Configure and build

From the **x64 Native Tools Command Prompt for VS 2022**:

```
set FFMPEG_DIR=C:\ffmpeg
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

(add `-DDISPLAYCLUSTER_CUDA=OFF` to the first line without the CUDA Toolkit)

The first configure builds the full vcpkg dependency set from source
(94 packages, mostly Boost and Qt6); expect anywhere from about 25 minutes on
a fast many-core machine to well over an hour. vcpkg caches build artifacts
machine-wide afterward (`%LOCALAPPDATA%\vcpkg\archives`), so subsequent
configures — even in a fresh `build\` directory, or from another project
using the same package/version/feature/triplet combination — are fast.

Things to check in CMake's output:
- `remote API: HTTPS supported (OpenSSL 3.x)`
- `Found MPI: TRUE` (if not, the prompt predates the MS-MPI SDK install)
- `Building without CUDA: movies decode in software` appears only when CUDA
  is off

To turn CUDA on later, install the CUDA Toolkit, open a **new** prompt (so
`CUDA_PATH` is set), and run the same configure line with
`-DDISPLAYCLUSTER_CUDA=ON`; with the vcpkg packages already built that takes
seconds. As of CUDA 13, `cuda.lib` is a small static loader that opens
`nvcuda.dll` when CUDA is first used, not an import library, so
`dumpbin /dependents` won't list `nvcuda.dll`; that's expected.

#### Running

The build copies the runtime DLLs (Qt, Boost, OpenSSL, FFmpeg, and the
libraries Qt's plugins need) next to `displaycluster.exe`, so `C:\ffmpeg\bin`
needn't be on `PATH`; `msmpi.dll` comes from the MS-MPI runtime. Qt's plugin
subdirectories (platform integration, image format loaders) are **not**
copied, so `QT_PLUGIN_PATH` needs to be set explicitly:

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

With `mpiexec -n 2` that's rank 0 (the control window) plus one render rank
with one 800×600 windowed tile. The remote-control web UI is then at
<http://localhost:1910/ui/> (the API listens on `127.0.0.1` only until
`DISPLAYCLUSTER_API_TOKEN` is set). Its media browser starts in your home
folder; to offer other folders instead, set `DISPLAYCLUSTER_MEDIA_DIRS`,
separating entries with `;` on Windows (`:` elsewhere), e.g.
`set DISPLAYCLUSTER_MEDIA_DIRS=media=D:\wall-media;C:\src\DisplayCluster\examples`.

#### Hardware movie decode, and laptops with two GPUs

With CUDA on, NVDEC decodes each movie on the NVIDIA GPU and CUDA copies the
frames straight into the tile's OpenGL textures. That last step only works
when the NVIDIA GPU also does the drawing. Laptops with hybrid graphics (an
integrated Intel or AMD GPU plus an NVIDIA one) hand new apps to the
integrated GPU by default, and the render rank then logs:

```
Movie::render: can't share GL textures with CUDA (...; GL renderer AMD Radeon(TM) 610M), so hardware-decoded frames will be copied through host memory
```

Movies still play, but every frame takes a round trip through system memory
to the other GPU. To fix it, give `displaycluster.exe` the NVIDIA GPU:
**Settings → System → Display → Graphics → Add an app → Desktop app**, browse
to `build\Release\displaycluster.exe`, then **Options → High performance**
(or, in the NVIDIA Control Panel, **Manage 3D settings → Program Settings →
High-performance NVIDIA processor**). Rebuilding to a different path needs
the setting again. Desktop machines driving monitors from the NVIDIA card
don't need any of this.

To check that a playing movie is on the GPU, run `nvidia-smi dmon -s u`:
the `dec` column should be above 0. `nvidia-smi` should list the render rank's
`displaycluster.exe` with type `C+G` (CUDA and graphics on that GPU), and
the "can't share GL textures" line shouldn't appear. Task Manager's GPU tab
shows the decoder's load as "Video Decode".

### Ubuntu

Package names below are current as of a recent Ubuntu release; adjust via
`apt search` if a name has changed on your version.

```bash
sudo apt install build-essential cmake pkg-config git \
    qt6-base-dev qt6-svg-dev \
    libboost-serialization-dev libboost-date-time-dev libboost-iostreams-dev \
    libturbojpeg0-dev libssl-dev \
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
copies the DLLs next to the exe but not Qt's plugin subdirectories). From the
`streamer` directory:

```
set QT_PLUGIN_PATH=%CD%\build\vcpkg_installed\x64-windows\Qt6\plugins
build\Release\DesktopStreamer.exe
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
    libturbojpeg0-dev
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

```bash
brew install cmake qt boost jpeg-turbo
```

Homebrew's Qt isn't on CMake's default search path, so tell CMake where it
is:

```bash
cmake -S streamer -B build-streamer -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
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

Clone the repo somewhere — we'll refer to this as the source directory.
To build the DC image, run this from the source directory itself (not from
inside `docker/` — the Dockerfile `COPY`s the checkout it lives in, rather
than cloning a fresh copy from GitHub, so the build context has to be the
repo root):

```bash
./build-sif.sh
```

It runs three steps: `docker build -f docker/Dockerfile -t displaycluster .`,
then `docker save` of the image to an archive in a temporary directory, then
`apptainer build displaycluster.sif docker-archive:<that archive>`. It
deletes the archive (about 2.5 GB) when it's done, and the Docker images that
earlier builds left behind.

This creates **displaycluster.sif**, with DisplayCluster installed under
`/usr/local` in the image and its source, including `scripts/`, in
`/usr/local/src/DisplayCluster`.

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

### Installing and starting it

Nothing needs installing on the wall's machines but apptainer, the NVIDIA
driver and a matching Open MPI (below). The wall needs three files, readable
at the same paths on every machine (a shared home directory works):

- **displaycluster.sif**
- the wall's **configuration.json**, describing its tiles and which node
  draws each one;
- a copy of **scripts/startdisplaycluster**, edited for the site.

`startdisplaycluster` is the one file a wall needs outside the image. Copy it
somewhere on the `PATH` (e.g. `~/bin`) and edit the settings at its top:
where the sif and `configuration.json` are, the content directory, the
screensaver, extra `mpirun` arguments, and so on. A copy is in every image,
so a site needs no checkout of the repo to get one:

```bash
apptainer exec displaycluster.sif cat /usr/local/src/DisplayCluster/scripts/startdisplaycluster > ~/bin/startdisplaycluster
```

Every setting is an environment variable, and one already set in the
environment wins. Every `DISPLAYCLUSTER_*` variable is passed on to all of the
wall's processes, so others (see [doc/remote-control.md](doc/remote-control.md))
can be added to the file or set before running it.

In some cases (like Rattler here at TACC) Open MPI doesn't find the correct
network interface to use. Name it in `DISPLAYCLUSTER_MPIARGS` in
`startdisplaycluster`, e.g. `--mca btl_tcp_if_include enp0s25`.

[doc/setup.md](doc/setup.md) covers the rest: preparing the display nodes'
X servers, writing `configuration.json`, and a first run.

The Python client needs no install either; use it from a checkout:

```bash
export PYTHONPATH=/path/to/DisplayCluster/python
python3 examples/run_script.py
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

## Remote control

The wall can be controlled from other machines: from a web browser at
`http://wallhost:1910/ui/` (which also installs as a full-screen app on an
iPhone or iPad), from Python scripts with `python/DC.py`, and from anything
that speaks HTTP, through an HTTP/JSON API served by the master process.

To reach it from another machine, give the wall a token — in
`DISPLAYCLUSTER_API_TOKEN`, or `~/.displaycluster/api_token` for the account
that runs it — and open port 1910 in its firewall (or use an SSH tunnel).
With a certificate in `~/.displaycluster/api_cert.pem` and `api_key.pem`, it
serves all of this over HTTPS instead.

A browser extension for Chrome and Edge, in `extension/`, shows a browser tab
on the wall, live, through the same port.

[doc/remote-control.md](doc/remote-control.md) covers setting it up, the web
UI, the phone app, the browser extension, scripting with `DC.py`, and the API.

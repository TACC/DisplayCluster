# Building DisplayCluster (Qt6)

Native build instructions for Windows 11 and Ubuntu. Containers are no longer
required for either platform — the old apptainer/container workflow described
in `README` was necessitated by Qt4 only being available via a PPA on old
Ubuntu; Qt6 is available natively everywhere this now targets.

## Windows 11

### Toolchain

- Visual Studio 2022 (Community or higher) with the **"Desktop development
  with C++"** workload — provides MSVC, the Windows SDK, and the "x64 Native
  Tools Command Prompt for VS 2022". **Builds must be run from this specific
  prompt**, not a plain terminal — `cl.exe` needs its environment
  (`INCLUDE`/`LIB`/`PATH`) initialized, which only happens there (or after
  manually running `vcvarsall.bat x64`).
- Git for Windows
- CMake 3.16+ (bundled with VS2022, or install standalone)

### Package manager

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

### Installed manually (not vcpkg-managed)

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

### vcpkg-managed dependencies

Everything else — Qt6 (`qtbase` with `widgets`/`network`/`opengl` features,
plus `qtsvg`), Boost (`serialization`, `date-time`, `iostreams`, `algorithm`,
`tokenizer`, `smart-ptr`), and `libjpeg-turbo` — is declared in `vcpkg.json`
and installs automatically during CMake configure. No manual step needed.

### Configure and build

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

### Running

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

## Ubuntu

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

### Configure and build

Build out-of-tree, e.g. as a sibling of the repo rather than inside it —
the Dockerfile COPYs the whole repo directory into the image, and an
in-tree `build/` (even though `.gitignore`/`.dockerignore` both exclude
it from tracking/COPY) is easy to end up dragging along by accident:

```bash
cmake -S . -B ../build
cmake --build ../build -j$(nproc)
```

### Running

```bash
export DISPLAYCLUSTER_DIR=/path/to/DisplayCluster
export DISPLAYCLUSTER_CONFIG=/path/to/DisplayCluster/configuration.json
export DISPLAYCLUSTER_TIMEOUT=3600
mpirun -np 2 ./build/displaycluster
```

Same `configuration.json` and `DISPLAYCLUSTER_TIMEOUT` notes as the Windows
section above apply.

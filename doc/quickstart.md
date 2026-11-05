# DisplayCluster quickstart

The shortest path from a checkout to a working wall, and to the machines
that control it. Each step links to the full instructions in
[setup.md](setup.md), [content.md](content.md) and
[remote-control.md](remote-control.md).

For people using a wall that's already running, there's a one-page
[cheat sheet](cheatsheet.md) to hand out.

A wall is a head node, which runs the control process and serves the web UI,
and display nodes, each drawing some of the wall's tiles. One machine can be
both: the smallest wall is a single computer and its monitors.

## 1. What you need

- Linux with X11 and the NVIDIA driver on every machine of the wall, with an X
  server running on each display node
  ([Preparing the display nodes](setup.md#preparing-the-display-nodes)).
- Open MPI 4.1, and passwordless SSH from the head node to every display node,
  for the account that runs the wall.
- A filesystem every machine of the wall sees at the same path, such as a
  shared home directory. Not needed on a single machine.

The full list is in [What every wall needs](setup.md#what-every-wall-needs).

## 2. Build

There are two kinds of build. Both run the same program
([Choosing a build](setup.md#choosing-a-build)).

**Container, for a wall.** One image file holds the program and everything it
needs, so the wall's machines need only apptainer, the NVIDIA driver and
Open MPI. Build it on any Linux machine with Docker and apptainer, straight
into the wall's DisplayCluster directory (next section):

```bash
git clone https://github.com/TACC/DisplayCluster.git && cd DisplayCluster
./build-sif.sh -r /shared/DisplayCluster
```

The first build takes 15–30 minutes, later ones a few. If the build machine
can't see that directory, build without `-r` and install the image on the
wall with `install-displaycluster`
([Container build](setup.md#container-build)).

**Native, for a Mac, a single Linux machine, or development.** Install the
[dependencies](setup.md#dependencies), then build and install into the
directory:

```bash
cmake -S . -B ../dc-build -DCMAKE_INSTALL_PREFIX=/shared/DisplayCluster
cmake --build ../dc-build -j8
cmake --install ../dc-build
```

([Native build](setup.md#native-build).) On a Mac, build in the checkout's
`build/` instead and run it with `scripts/run-macos.sh`; see
[macOS](setup.md#macos).

## 3. The DisplayCluster directory

Either build leaves the wall's files in one directory:

```
DisplayCluster/
  bin/            startdisplaycluster, killdc, install-displaycluster, cyclestates, ...
  images/         the images (container), displaycluster.sif linking to the current one
  python/         DC.py, for scripts
  wall.json       the wall's layout: edit this
  site.sh         your settings, if any: make this
  Content/        media and saved layouts; the web UI browses here
```

Put its `bin/` on the wall account's `PATH`. The scripts find the rest of the
directory from where they are, and need no editing
([The DisplayCluster directory](setup.md#the-displaycluster-directory)).

## 4. Describe the wall

**`wall.json`** says how many tiles the wall has, their size, the gaps
between them, and which host and X display draws each one. The first install
writes an example to edit
([Describing the wall](setup.md#describing-the-wall-configurationjson)).

**`site.sh`** holds anything you change from the defaults, as shell
variables. For example:

```bash
DISPLAYCLUSTER_TIMEOUT=3600        # screensaver after an hour idle
DISPLAYCLUSTER_MPIARGS="-bind-to none --mca btl_tcp_if_include enp0s25"
```

Updates replace the scripts but never `wall.json`, `site.sh` or `Content/`.
The settings are listed under
[The launcher](setup.md#the-launcher-startdisplaycluster).

## 5. Start it, check it, stop it

On the head node, as the wall's account:

```bash
startdisplaycluster
```

The control window opens on the head node and the tiles on the display nodes.
Turn on **View → Show Test Pattern**: each tile shows a grid labelled with its
host and position, and its lines should run straight across the whole wall
([First run](setup.md#first-run)).

Ctrl-C stops the wall. If anything is left running on the nodes after a crash,

```bash
killdc
```

stops it everywhere in `wall.json` (`killdc -n` shows what it would stop).
To keep it running after you log out, or start it at boot or on a schedule,
see [Running it unattended](setup.md#running-it-unattended).

## 6. The web UI

Without a token, the wall can be controlled only from the head node itself.
To control it from anywhere, give it a token and restart it:

```bash
mkdir -p ~/.displaycluster && chmod 700 ~/.displaycluster
python3 -c "import secrets; print(secrets.token_hex(32))" > ~/.displaycluster/api_token
chmod 600 ~/.displaycluster/api_token
```

Then open `http://headnode:1910/ui/` in a browser, enter the token, and press
**Take control**. Port 1910 must be open to the people who'll use it. For
HTTPS (so the token isn't sent in the clear, and so Android can install the
web UI as an app), add a certificate and key in the same folder
([Setting up the wall](remote-control.md#setting-up-the-wall)). The web UI also
works on a phone or tablet
([On a phone or tablet](remote-control.md#on-a-phone-or-tablet)).

## 7. Your content

Put each project in its own folder in `Content/`, for example
`Content/talks/morning/`. Open its files on the wall from there (the web UI's
file browser shows `Content/`), arrange them, and save the layout, a `.dcx`
state file, in the same folder with **Save state…**. Files in the layout's
folder are saved relative to it, so the folder can be moved, renamed, or
copied to another wall as a whole ([content.md](content.md)).

## 8. Client machines

People control and show things on the wall from their own machines. They all
need the same four things from whoever runs the wall: its **host name**, its
**port** (1910), its **token**, and, if the wall uses HTTPS with a certificate
of its own rather than one from a public CA, a **copy of that certificate**.

**Build the client tools** on a presenter's machine (Linux, macOS or Windows).
This needs only Qt 6, Boost and libjpeg-turbo: none of the wall's
dependencies.

```bash
cmake -S . -B build-client -DDISPLAYCLUSTER_CLIENT_ONLY=ON -DCMAKE_INSTALL_PREFIX=$HOME/DisplayCluster
cmake --build build-client -j8 && cmake --install build-client
```

On a Mac with Homebrew's Qt, add `-DCMAKE_PREFIX_PATH="$(brew --prefix qt)"`
to the first line. On Windows, add vcpkg's toolchain file, as in
[DesktopStreamer](setup.md#desktopstreamer).

It installs DesktopStreamer (`DesktopStreamer.app` on a Mac), `python/` with
`DC.py`, `bin/cyclestates`, and the Chrome extension in `extension/`. To hand
DesktopStreamer to people without a build setup, package it first
([DesktopStreamer](setup.md#desktopstreamer)).

- **Python scripts** (`DC.py`): put the token in `~/.displaycluster/api_token`
  on the client, and connect with `DC('headnode')`; add `https=True` for an
  HTTPS wall ([Scripting with DC.py](remote-control.md#scripting-with-dcpy)).
- **The Chrome extension**, for showing a browser tab: load the `extension/`
  folder at `chrome://extensions` (Developer mode, **Load unpacked**), and
  enter the wall's address and token in its options
  ([Streaming a browser tab](remote-control.md#streaming-a-browser-tab)).
- **DesktopStreamer**, for showing a screen or part of one: enter the head
  node's name as **Hostname** and press **Share**. It uses port 1701, which needs no token, so open that
  port only to networks you trust ([Showing a screen](setup.md#showing-a-screen-or-a-browser-tab)).

## 9. Updating

```bash
cd DisplayCluster && git pull && ./build-sif.sh -r /shared/DisplayCluster
```

The new image is installed as the build finishes; restart the wall
(`killdc`, then `startdisplaycluster`) to run it. To go back, install an older
image from `images/` again with `install-displaycluster`
([Updating](setup.md#updating)). For a native build, rebuild and run
`cmake --install` again.

# Managing content and layouts

This guide covers where a wall's content can live, and how saved layouts
(`.dcx` state files) refer to it. Layouts break when the files they name
move, so it explains:
- how a state file's paths are read;
- what moving files does to them;
- how to keep a layout and its content together with `localize-dcx`;
- how to find and repair broken layouts with `check-dcx`.

Setting up the wall is in [setup.md](setup.md), and the web UI and `DC.py` are
in [remote-control.md](remote-control.md).

## Where content can live

The wall shows content by reading files from disk. It opens nothing by
uploading or copying, so a file has to be somewhere every one of the wall's
processes can read it.

**Every node reads the file itself, by the same path.** The control process
and each display node's process open the file separately, using the same path.
A file in `/home/vislab/Content/talk/map.png` on the control host has to be at
`/home/vislab/Content/talk/map.png` on every display node too. In practice
content lives on a filesystem shared by all of them, such as the home
directories. A file on one machine's local disk shows on that machine's tiles
only, and every other tile shows the error image.

This is what a shared filesystem already gives you. On a cluster whose home
directories (or other folders) are on NFS, or an equivalent such as Lustre,
GPFS or BeeGFS, every node sees the same files at the same paths, so content
put there just works and nothing needs copying to the nodes. Stallion's `/home`, for
example, is NFS. Only content on a disk that isn't
shared needs thought: copy it to the same path on every node, or move it to a
shared folder.

**In the container, the folder has to be visible inside the image.** The
container (apptainer) sees your home directory, `/tmp` and the folder the wall
starts in, plus whatever the site's apptainer configuration adds. A folder
elsewhere, such as `/scratch` or `/corral`, has to be bound in. Name it in
`APPTAINER_BINDPATH`, separated by commas, before starting the wall, or set it
in `startdisplaycluster`. The launcher passes it on to every node:

```bash
export APPTAINER_BINDPATH=/scratch,/corral
```

A folder that isn't bound behaves like a missing file: the window shows the
error image.

**Folders the web UI browses.** The web UI's file browser shows only the
folders in `DISPLAYCLUSTER_MEDIA_DIRS`. The launcher sets this to
`content=$CONTENT_DIR`, which is `~/Content` by default. Add more as
`name=path` pairs separated by colons:

```bash
export DISPLAYCLUSTER_MEDIA_DIRS="content=/home/vislab/Content:scratch=/scratch/vislab/media"
```

This limits browsing only. A state file, the control window or `DC.py` can
name any file the wall can read.

**The wall's working directory is `CONTENT_DIR`.** The launcher starts the wall
there. A relative path given to `DC.py`'s `open()` is resolved against it, but
the page and the control window always use full paths. Give `DC.py` full paths
too.

**Where state files are kept.** Layouts saved and loaded from the web UI or
`DC.py` live in the state directory, `DISPLAYCLUSTER_STATE_DIR`, which is
`~/.displaycluster/states` by default. It can have folders. The web UI and
`DC.py` can't load or save outside it. The control window's Load State and Save
State can use any folder. To keep layouts with the media, point it into the
content folder, for example `DISPLAYCLUSTER_STATE_DIR=$HOME/Content/states`.

## What a state file holds

A `.dcx` file is a small XML file with one `ContentWindow` for each window.
Each one holds the window's content path (`URI`), its name, its place on the
wall and its zoom:

```xml
<state>
  <version>1</version>
  <ContentWindow>
    <URI>/home/vislab/Content/talk/map.png</URI>
    <name>map</name>
    <x>0.1</x> <y>0.1</y> <w>0.4</w> <h>0.5</h>
    ...
  </ContentWindow>
</state>
```

The content isn't in the state file; only its path is. Loading a state opens
each path afresh.

### How paths in a state file are read

- **A full path** (starting with `/`) means exactly that file, wherever the
  state file is. The wall always saves full paths.
- **A relative path** is relative to the folder the state file is in. In
  `/home/vislab/.displaycluster/states/talks/morning.dcx`, the path
  `morning/map.png` means
  `/home/vislab/.displaycluster/states/talks/morning/map.png`.
  That's so whatever loads it (the web UI, `DC.py` or the control window).
- **Paths are taken literally.** A `file://` URI or a `%20`-encoded name, as
  some tools write, is not decoded. The wall looks for a file with that exact
  name and doesn't find it.
- **An image pyramid's `.pyr` file names its tile folder the same way.** A
  full path is used as it is, and a relative one is relative to the `.pyr`
  file's folder. File → Compute Image Pyramid writes a full path.

A window whose file can't be found is still opened, in the same place, but it
shows the error image (a red warning sign). The wall's log says which file:
`could not find file ...`.

Pixel streams, such as a desktop or a browser tab being streamed, are saved
too, but a state can't bring them back. Their windows open showing the error
image.

## Moving things, and what breaks

Because a saved state names full paths, its windows depend on each file
staying exactly where it was.

| You... | Then... |
|---|---|
| move or rename a content file | every state that uses it shows the error image there |
| move or rename the folder it's in | the same, for everything in that folder |
| move a state file (saved by the wall) | nothing breaks: its paths are full ones |
| move a state file with relative paths, without its content | those windows break |
| move a folder holding a state file and its content, all named relatively | nothing breaks |
| copy a state file to another wall | it works only if every file is at the same path there |
| move an image pyramid's tile folder | the `.pyr` file's full path no longer finds it; edit it, or use a relative path |
| delete content that a state uses | that window breaks, and only a backup brings it back |

**The safe arrangement is a self-contained folder.** Put a state file and its
content in one folder, with the state naming the content relatively. That
folder can be moved, renamed, backed up or copied to another wall as a whole.
`localize-dcx` makes such a folder from any state.

## localize-dcx: gathering a layout into one place

`localize-dcx` takes a state file and gathers everything it shows into one
folder. It copies each file into a `Content/` folder, and writes a new state
file beside it that names the content relatively:

```
talks/morning/
    morning.dcx           <- names Content/map.png, Content/survey.pyr, ...
    Content/
        map.png
        intro.mp4
        survey.pyr        <- names its tiles relatively: "survey.pyramid" ...
        survey.pyramid/   <- the pyramid's tiles
```

```bash
localize-dcx [folder] file.dcx
```

`folder` is where the new state file and its `Content/` go; it's made if
needed, and it defaults to the current folder. The original state file and
content aren't changed.

The usual way to use it: lay out a talk on the wall from wherever its files
are, and save it. Then gather it into its own folder in the state directory, so
it's listed under Load state… and can't be broken by later tidying:

```bash
cd ~/.displaycluster/states
localize-dcx talks/morning morning.dcx
```

Load `talks/morning/morning.dcx` from the web UI to check it. You can then
delete the original `morning.dcx`.

What it does, in detail:
- **It reads paths the way the wall does**, so it gathers what the wall would
  show.
- **If anything is missing, it writes nothing.** It lists what it couldn't
  find. Fix those first, with `check-dcx` or by hand, so the copy isn't
  missing windows.
- **Files are copied.** An image pyramid's tiles are hard linked rather than
  copied, which takes no extra space. They're copied instead if the new folder
  is on another filesystem. Either way, deleting the original pyramid later
  doesn't harm the copy.
- **Each file keeps its name.** Two different files with the same name, from
  different folders, get different names (`map.png`, `map-2.png`).
- **Running it again updates the copy.** Unchanged files are skipped. If
  `folder/file.dcx` exists already and the new one differs, the old one is kept
  as `file.dcx.orig`.
- **It doesn't need the wall to be running**, or the container: it runs on
  the host, with Python 3 ([below](#running-the-scripts)).

To copy a gathered folder to another wall, copy the whole folder and load it
from that wall's state directory. `rsync -aH` keeps the pyramid tiles hard
linked:

```bash
rsync -aH ~/.displaycluster/states/talks/morning/ otherwall:.displaycluster/states/talks/morning/
```

## check-dcx: finding and repairing broken layouts

`check-dcx` checks that the wall can open every window in a state file. It
reads paths exactly as the wall does: literally, and a relative one relative
to the state file's folder. For a `.pyr` file it also checks that the tiles are
there.

```bash
check-dcx [-f] file.dcx
```

For each problem it prints one line:

| It says | Meaning |
|---|---|
| `'…' not found` | no such file, and no file of that name anywhere under the state file's folder |
| `'…' not found ... might be '…'` | a file of that name is under the state file's folder; it may have moved there |
| `'…' is in a form the wall can't read; it should be '…'` | a `file://` URI or `%`-encoded name, and the file it means exists |
| `pyramid '…': …` | the `.pyr` file's tile folder is missing or empty, or the file is malformed |

With `-f`, it fixes the state file, but only if every problem can be fixed;
otherwise nothing changes. A pyramid problem, or a file it can't find, has to
be fixed by hand. A file found under the state file's folder is written
relatively, so the folder stays movable; anything else gets a full path. The
original is kept as `file.dcx.orig` (`file.dcx.orig.1`, and so on, if that
exists). Not ending in `.dcx`, these aren't listed as states.

It searches only under the state file's own folder. That makes it most useful
for a state kept with its content, or one whose content was moved into a
folder beside it. For content moved somewhere else entirely, edit the paths in
the state file, which is plain text. You can also reopen the files on the wall
and save the state again.

It exits 0 if the state file was fine or was fixed, and 1 otherwise. That lets
it check a whole state directory:

```bash
find ~/.displaycluster/states -name '*.dcx' | while read -r f; do
    check-dcx "$f" > /dev/null || echo "broken: $f"
done
```

## Running the scripts

Both scripts run on the host, like any other command: they're plain Python 3,
using only its standard library, and don't talk to the wall, so they don't
need the container. Run them on any machine that sees the files, usually the
head node.

Put a copy where your shell finds it, such as `~/bin`. From a checkout, copy
them from `scripts/`. With only the image, copy them out of it:

```bash
mkdir -p ~/bin
for s in check-dcx localize-dcx; do
    apptainer exec displaycluster.sif cat /usr/local/bin/$s > ~/bin/$s
    chmod +x ~/bin/$s
done
```

Copy them again after installing a new image, to get any fixes.

On a machine without Python 3, run them in the container instead. It sees your
home directory and the current folder; a folder elsewhere needs `--bind`:

```bash
apptainer exec displaycluster.sif check-dcx talk.dcx
```

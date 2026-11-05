# DisplayCluster cheat sheet

For people using a wall that's already set up. There are two ways to drive
it, and they do the same things:

- **At the console:** the head node's own screen, where the wall is started
  from an xterm and shows its **control window**, a picture of the wall you
  work in with the mouse.
- **From a browser:** the **web UI**, on any computer, phone or tablet. Ask
  whoever runs the wall for its **address** (such as
  `rattler.tacc.utexas.edu`) and its **token**.

Setting a wall up is in [quickstart.md](quickstart.md); everything here is
covered in more detail in [setup.md](setup.md#using-the-control-window),
[remote-control.md](remote-control.md) and [content.md](content.md).

## At the console: start and stop the wall

Log in to the head node's desktop as the wall's account, and open an xterm.

| | |
|---|---|
| Is it running? | `pgrep -u $USER -l mpirun`: no output means it isn't |
| Start it | `startdisplaycluster` |
| Stop it | **File → Quit** in the control window, or Ctrl-C in the xterm |
| Clean up after a crash | `killdc` stops anything left on any of the wall's machines (`killdc -n` shows what it would stop) |
| Keep it running after you log out | `tmux new -d -s wall "DISPLAY=$DISPLAY startdisplaycluster"`; watch it with `tmux attach -t wall` (Ctrl-B then D leaves it running); stop it with `tmux kill-session -t wall` |

`startdisplaycluster` opens the control window on the screen the xterm is on,
and the wall's tiles on the display nodes. Leave the xterm open: the wall's
messages appear there, and closing the xterm stops the wall. Start the wall
only once; if it's already running, use the control window that's up, or the
web UI.

## At the console: the control window

The control window is a scaled-down picture of the whole wall, with a list
of the open windows beside it. Whatever you do in it happens on the wall.

| To | Do |
|---|---|
| bring a window to the front | click it |
| move it | drag it |
| resize it | drag its bottom-right corner, or scroll over it |
| close it | click its top-right corner |
| pan and zoom inside it | double-click it (its border changes); then drag to pan, scroll or right-drag to zoom; double-click again when done |

| Menu | Item | What it does |
|---|---|---|
| File | Open Content | opens a file in a new window |
| | Open Contents Directory | opens every file in a folder, in a grid (it asks for columns and rows) |
| | Clear | closes every window |
| | Save State, Load State | saves the layout to a `.dcx` file, and loads one |
| | Compute Image Pyramid | prepares a very large image for fast display |
| | Quit | stops the wall |
| View | Show Test Pattern | a labelled grid on every tile, for checking the wall's alignment; turn it off again the same way |
| | Show Window Borders, Show Content Labels, Constrain Aspect Ratio, ... | display options, shared with the web UI's **⋯** menu |

The file dialogs start in the wall's `Content/` folder. While someone has
control through the web UI or a script, the control window shows a banner
saying who, and stops responding to the mouse; **Take control** in the banner
takes it back.

## From a browser: the web UI

| | |
|---|---|
| Open the web UI | `https://wallhost:1910/ui/` (or `http://`, if the wall has no certificate) |
| Sign in | enter the token; tick **Remember on this device** to keep it |
| Change anything | **Take control** in the top bar; **Release** when you're done |
| Just watch | no control needed; any number of people can watch at once |
| On a phone or tablet | the same address; **Share → Add to Home Screen** in Safari makes it an app ([more](remote-control.md#on-a-phone-or-tablet)) |
| Forget the token | **⋯ → Disconnect and forget token** |

Only one person or script has control at a time, and the top bar says who.
Taking it from someone asks you to confirm first. If the page goes into the
background or your computer sleeps, your control lapses after 30 seconds; it
comes back by itself when you return within 10 minutes, if nobody else has
taken it meanwhile.

## Arrange windows in the web UI

| To | Mouse | Touch |
|---|---|---|
| select (and bring to front) | click | tap |
| move | drag | tap, then drag |
| resize | drag a corner | drag a corner |
| scale | scroll over it | pinch it |
| close | ✕ on the selected window | ✕ on the selected window |
| pan and zoom inside it | double-click, then drag and scroll; Esc when done | double-tap, then drag and pinch; double-tap when done |
| zoom your view of the wall | ctrl+scroll | pinch empty space |
| reset your view | double-click empty space | double-tap empty space |

The panel on the right (the inspector) acts on the selected window:
**Fill wall** / **Restore**, **To front**, **Hide** / **Show**, **Close**,
**Adjust view** / **Reset view**, a name, and exact position and size in
tiles. The list on the left has every window, front first, with an eye to
hide or show each; **Close all windows…** clears the wall.

## Show your files

**Put them on the wall's filesystem first.** The wall opens files from its
own disks, not from your computer. Each project goes in its own folder under
the wall's `Content/` folder. With an account on the wall's head node:

```bash
rsync -av mytalk/ wallhost:/shared/DisplayCluster/Content/talks/mytalk/
```

(Otherwise, ask whoever runs the wall to copy them in.)

**Open them** with **Open…** in the web UI: go into the folder and click a
file. **Open folder as grid** opens everything in the folder, tiled across the
wall. At the console, use **File → Open Content** or **Open Contents
Directory**.

| The wall shows | Files |
|---|---|
| images | `.png`, `.jpg`, `.tif`, `.gif`, `.bmp`, `.webp` and the other formats Qt reads |
| movies | `.mp4`, `.mov`, `.mkv`, `.avi`, `.mpg`, `.flv`, `.wmv` |
| drawings | `.svg` |
| very large images | `.pyr` image pyramids, made from a large image in the control window ([more](setup.md#very-large-images-image-pyramids)) |

A window with a red warning sign means the wall couldn't read its file: it's
been moved or renamed, or isn't on the wall's shared filesystem.

## Save and load layouts

A layout (a `.dcx` state file) records which files are open, and where.

| | Web UI | Control window |
|---|---|---|
| Save | **Save state…**: go into your project's folder, give it a name, **Save** | **File → Save State**, in your project's folder |
| Load | **Load state…**: go into the folder, click the layout | **File → Load State** |
| New folder | **New folder** in Save state…, or a name with a slash: `talks/mytalk` | the file dialog's own new-folder button |

**Save a layout in the folder with its files.** The wall then records those
files relative to the layout, so the whole folder can be moved, renamed, or
copied to another wall, and still opens. Files from elsewhere are recorded by
their full path, and break if they move ([more](content.md#moving-things-and-what-breaks)).

## Show your screen or a browser tab

| | Browser extension | DesktopStreamer |
|---|---|---|
| Shows | one browser tab (Chrome or Edge) | your screen, or part of it |
| Set up once | load the `extension/` folder at `chrome://extensions` (Developer mode → **Load unpacked**), then enter the wall's address and token in its options | install DesktopStreamer (ask for a copy) |
| Start | the extension's button → **Start streaming**, or **Alt+Shift+W** | **Hostname**: the wall's address; **Stream name**: yours; **Share** |
| Stop | **Stop streaming**, or close the tab | **End Sharing** |
| Choose the area | the whole tab | **Options → Show Rectangle**, then drag and resize it |

Each stream becomes a window on the wall that you can move like any other.
For a sharp picture, make the browser window or the shared area large. The
extension's button shows a red **!** if it can't stream; its popup says why.
On a Mac, DesktopStreamer needs Screen Recording permission (System Settings →
Privacy & Security) the first time. DesktopStreamer isn't encrypted, so don't
use it for anything confidential on a network you don't trust
([more](remote-control.md#streaming-a-browser-tab)).

## Script it

`DC.py`, from the wall's `python/` folder, needs only Python 3. Put the token
in `~/.displaycluster/api_token` on your computer, then:

```python
from DC import DC
dc = DC('wallhost', https=True)          # https=True for an https:// wall
w = dc.open('/shared/DisplayCluster/Content/talks/mytalk/map.png', x=0, y=0, w=2, h=1)
dc.fill(w)                               # fill the wall; dc.fill(w, False) puts it back
dc.loadState('talks/mytalk/mytalk.dcx')  # named relative to Content/
dc.releaseControl()                      # let others use the wall
```

Positions and sizes are in tiles, from the wall's top-left corner, and file
paths are the wall's, not your computer's
([more](remote-control.md#scripting-with-dcpy)).

## Sleep and wake

The wall sleeps (showing its screensaver) after a while with nothing
happening. Any change wakes it, and so does a click in the web UI. **Sleep**
in the web UI puts it to sleep straight away. Streaming keeps it awake.

## When something's wrong

| You see | What to do |
|---|---|
| Dragging does nothing in the web UI | you don't have control: **Take control** |
| The control window has a banner and ignores the mouse | someone has control through the web UI or a script: **Take control** in the banner |
| `startdisplaycluster` says it can't read a file | the wall isn't set up as it expects; tell whoever runs it |
| The tiles stay dark or show an old picture after a crash | `killdc`, then `startdisplaycluster` |
| "Take control of the wall first." though you had it | yours lapsed and someone else took it, or you were away for over 10 minutes: take it again |
| A red warning sign in a window | its file has moved or isn't on the wall's shared filesystem; open it again from its new place |
| A layout loads with warning signs | its files moved; keep layouts in the folder with their files |
| The web UI doesn't load | check the address and `https://` or `http://`; the network or a firewall may be blocking port 1910 |
| The token is refused | it's changed: ask for the new one, and **⋯ → Disconnect and forget token** |
| DesktopStreamer's picture is black (Mac) | grant it Screen Recording permission, then restart it |
| The wall itself looks wrong (a tile dark, the picture misaligned) | tell whoever runs the wall |

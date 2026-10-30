# Remote control

DisplayCluster can be controlled from other machines: from a web browser
(including as an app on a phone or tablet), from Python scripts, and from
anything that speaks HTTP. All three go through the same HTTP/JSON API,
served by the wall's master process (rank 0). A browser extension uses the
same port to show a live browser tab on the wall.

- [Setting up the wall](#setting-up-the-wall)
- [The web UI](#the-web-ui)
- [On a phone or tablet](#on-a-phone-or-tablet)
- [Streaming a browser tab](#streaming-a-browser-tab)
- [Scripting with DC.py](#scripting-with-dcpy)
- [Control: one at a time](#control-one-at-a-time)
- [The screensaver](#the-screensaver)
- [API reference](#api-reference)

## Setting up the wall

The API is configured with environment variables, set where the wall is
started (`startdisplaycluster` passes them on to the wall's processes):

| Variable | Default | What it does |
|---|---|---|
| `DISPLAYCLUSTER_API_TOKEN` | read from `~/.displaycluster/api_token`, if that exists | a shared secret every client must present |
| `DISPLAYCLUSTER_API_PORT` | `1910` | the port the API and web UI are served on |
| `DISPLAYCLUSTER_API_BIND` | all interfaces (`0.0.0.0`) with a token; `127.0.0.1` without | the address the API listens on |
| `DISPLAYCLUSTER_API_CERT` | `~/.displaycluster/api_cert.pem`, if that exists | a certificate to serve HTTPS with; see [HTTPS](#https) |
| `DISPLAYCLUSTER_API_KEY` | `~/.displaycluster/api_key.pem`, if that exists | the certificate's private key |
| `DISPLAYCLUSTER_STATE_DIR` | `~/.displaycluster/states` | where state files are loaded from and saved to |
| `DISPLAYCLUSTER_MEDIA_DIRS` | your home directory | the directories the file browser shows |
| `DISPLAYCLUSTER_UI_DIR` | `<install prefix>/ui` | where the web UI's files are |
| `DISPLAYCLUSTER_TIMEOUT` | `5` | seconds without activity before the screensaver starts |
| `DISPLAYCLUSTER_SENSORS` | `0` | how many sensors the web UI offers for [sensor control](#sensors) |

`~` is the home directory of the account that runs the wall. Under apptainer
that's the same home directory as outside the container.

### The token

Without a token the API is only reachable from the wall's own machine. To
control the wall from anywhere else, generate a token:

```bash
python3 -c "import secrets; print(secrets.token_hex(32))"
```

and give it to the wall, either in the environment it's started from:

```bash
export DISPLAYCLUSTER_API_TOKEN="<the token>"
```

or in a file, readable only by you:

```bash
mkdir -p ~/.displaycluster && chmod 700 ~/.displaycluster
echo "<the token>" > ~/.displaycluster/api_token && chmod 600 ~/.displaycluster/api_token
```

Restart the wall. Its startup output should say
`remote API listening on 0.0.0.0:1910 (token required)`. Clients need the
same token — see each section below for where it goes. To revoke access,
change the token and restart the wall.

The token travels unencrypted over plain HTTP. On a network you don't
trust, turn on [HTTPS](#https), or use an SSH tunnel (below).

### HTTPS

Given a certificate, the wall serves everything over HTTPS (and WebSockets as
`wss://`) on the same port, instead of HTTP: `https://wallhost:1910/ui/`.
Plain HTTP is then refused. It needs a build with OpenSSL 3 (CMake's output
says `remote API: HTTPS supported`); a build without it, given a certificate,
doesn't start the API rather than serve it unencrypted.

Put the certificate (with any intermediate certificates after it) and its
private key, both PEM, in `~/.displaycluster`:

```bash
cp wallhost.crt ~/.displaycluster/api_cert.pem
cp wallhost.key ~/.displaycluster/api_key.pem && chmod 600 ~/.displaycluster/api_key.pem
```

or point `DISPLAYCLUSTER_API_CERT` and `DISPLAYCLUSTER_API_KEY` at them. Restart
the wall; it should say `remote API listening on 0.0.0.0:1910 (https, token
required)`.

A certificate from your institution's CA (or Let's Encrypt) is trusted by
browsers as is, with one catch: **ask for an RSA key, not ECDSA.** Some CAs
issue ECDSA certificates from a separate chain whose root not every browser
trusts. InCommon, for one, chains its ECDSA certificates to emSign ECC Root
CA - G3, which macOS trusts but Chrome doesn't. Safari and `curl` then accept
the wall's certificate while Chrome refuses it with
`NET::ERR_CERT_AUTHORITY_INVALID`. If the host's other web servers send HSTS,
there's no button to click past it. InCommon's RSA certificates chain to emSign Root
CA - G1, which Chrome trusts. To see which root a wall's certificate ends at:

```bash
echo | openssl s_client -connect wallhost.example.edu:1910 -showcerts 2>/dev/null | grep "i:" | tail -1
```

and compare it with a server on that host Chrome is happy with (port 443, say).
With certbot, `--key-type rsa` asks for RSA, and certbot remembers it for
renewals; to change an existing certificate:

```bash
sudo certbot certonly --cert-name wallhost.example.edu --key-type rsa --rsa-key-size 2048 --force-renewal
```

Without one, make a self-signed certificate, naming the wall's
host:

```bash
openssl req -x509 -newkey rsa:2048 -nodes -days 825 -subj /CN=wallhost.example.edu \
  -addext "subjectAltName=DNS:wallhost.example.edu" \
  -keyout ~/.displaycluster/api_key.pem -out ~/.displaycluster/api_cert.pem
chmod 600 ~/.displaycluster/api_key.pem
```

Browsers warn about a self-signed certificate until it's trusted: copy
`api_cert.pem` (not the key) to each client machine and add it to its trusted
certificates (Keychain Access on a Mac, `certmgr` on Windows, Settings →
General → VPN & Device Management and then Certificate Trust Settings on an
iPhone). Clicking through the warning lets the web UI work in that browser,
but the extension can't stream until the certificate is trusted.

#### Renewing the certificate

The wall reads its certificate only when it starts, so a renewed certificate
takes effect at the next restart. Until then it keeps serving the old one,
which is fine as long as that hasn't expired. To see when the one in use
expires:

```bash
openssl x509 -enddate -noout -in ~/.displaycluster/api_cert.pem
```

With Let's Encrypt, certbot renews about 30 days before expiry. A deploy hook
can copy each renewed certificate into place. Certbot runs it after every
renewal, as root. Save this as
`/etc/letsencrypt/renewal-hooks/deploy/displaycluster.sh`, with `vislab`
changed to the wall's account, and make it executable:

```bash
#!/bin/sh
# copies a renewed certificate to where DisplayCluster reads it
set -e
user=vislab
dir=$(getent passwd $user | cut -d: -f6)/.displaycluster
install -o $user -g "$(id -gn $user)" -m 644 "$RENEWED_LINEAGE/fullchain.pem" "$dir/api_cert.pem"
install -o $user -g "$(id -gn $user)" -m 600 "$RENEWED_LINEAGE/privkey.pem" "$dir/api_key.pem"
```

Use `fullchain.pem`, not `cert.pem`: browsers need the intermediate
certificate too. Test it with `sudo certbot renew --dry-run`. Then restart the
wall at a convenient time within the next 30 days.

The hook doesn't restart the wall itself, because a renewal can happen while
someone is using it. If the wall runs as a [systemd
service](setup.md#as-a-systemd-service) and a restart at a random moment is
acceptable, add `systemctl restart displaycluster` as the hook's last line.

### Reaching the wall

Clients connect to the wall's host on the API port, so that port has to be
open. On RHEL-family systems:

```bash
sudo firewall-cmd --permanent --add-port=1910/tcp && sudo firewall-cmd --reload
```

A client that times out, rather than being refused, is almost always being
blocked by a firewall. To check from another machine:

```bash
curl http://wallhost:1910/ui/
```

(or, on Windows, `Test-NetConnection wallhost -Port 1910` in PowerShell).

Where the port can't be opened, an SSH tunnel works from any machine you can
SSH from, whether the wall has a token or not:

```bash
ssh -L 1910:localhost:1910 you@wallhost
```

Then use `localhost` in place of `wallhost`: `http://localhost:1910/ui/`,
`DC('localhost')`. (With HTTPS, the browser will warn that the certificate is
for `wallhost`, not `localhost`.)

### Content and state directories

The file browser shows the directories in `DISPLAYCLUSTER_MEDIA_DIRS`,
separated like `PATH` (`:`, or `;` on Windows), each optionally given a name:

```bash
export DISPLAYCLUSTER_MEDIA_DIRS="content=/home/vislab/Content:movies=/scratch/movies"
```

An unnamed directory is named after its last component. Symbolic links are
followed only if they lead into one of these directories.

State files loaded and saved remotely are confined to
`DISPLAYCLUSTER_STATE_DIR`, and are named relative to it (`demo.dcx` or
`talks/demo.dcx`, not `/home/vislab/Content/demo.dcx`). The control window's own Load State and
Save State aren't limited this way.

Content in a state file is named by full path when the wall saves it. In a
state file written or edited by hand, a relative path is taken relative to
the state file's own folder, so a folder holding a talk's state and its
media (`talks/morning.dcx` naming `morning/map.png`, say) can be moved or
copied as a whole. This holds however the state is loaded. `localize-dcx`
makes such a folder from any state file, and `check-dcx` finds broken
references; see [content.md](content.md).

## The web UI

Open `http://wallhost:1910/ui/` in a browser and enter the token. With
**Remember on this device** ticked, the browser keeps it.

![The web UI](remote-ui.png)

The page shows the wall as it is right now, and follows every change,
whoever makes it: another browser, a script, or the control window.

### Taking control

Anyone with the token can watch. To change anything, press **Take control**
in the top bar — see [Control: one at a time](#control-one-at-a-time). The
top bar shows who has control. **Release** gives it back.

### The wall view

The wall is drawn dark, like the wall itself switched off, with its screens
and the gaps between them to scale. Each window shows a thumbnail of its
content (images, SVGs, and a frame from each movie), or an icon for its type
if there isn't one. Hidden windows are faded.

| To | With a mouse | With touch |
|---|---|---|
| select a window | click it (also brings it to the front) | tap it |
| move it | drag it | tap it, then drag it |
| resize it | drag a corner handle | drag a corner handle |
| scale it | scroll over it | pinch it |
| close it | the ✕ on the selected window | the ✕ on the selected window |
| pan and zoom its content | double-click it, then drag to pan and scroll or right-drag to zoom; double-click again or press Esc when done | double-tap it, then drag to pan and pinch to zoom; double-tap when done |
| zoom the view itself | ctrl+scroll (or pinch a trackpad) | pinch empty space |
| reset the view | double-click empty space | double-tap empty space |
| deselect | click empty space, or Esc | tap empty space |

When the wall keeps content in proportion (**Keep content aspect ratio** in
the options), resizing does too. Zooming the view only changes what your
screen shows, so it doesn't need control.

### The window list and inspector

The list on the left shows the windows front first; the eye beside each
hides or shows it. **Close all windows…** clears the wall.

The inspector shows the selected window's exact position, size and zoom (in
tiles: `(0, 0)` is the wall's top-left corner), and lets you change them,
rename the window, and:

- **Fill wall** — make it as large as fits, centered and in front. It then
  reads **Restore**, which puts it back where it was, and at its old place in
  the stacking order; so does selecting another window, or deselecting it.
  Moving or resizing a filled window keeps the new placement instead.
- **To front**, **Hide** / **Show**, **Close**.
- **Adjust view** — the same as double-clicking the window: pan and zoom its
  content. **Reset view** shows all of it again.

### Opening content

**Open…** browses the media directories: pick a location on the left, click a
folder to go into it, and **..** at the top of the list (or a name in the path
above it) to go back up. Sort by name, date or size, and open a file. **Open folder as grid**
opens the files in the current folder tiled across the wall, like the control
window's Open Contents Directory.

**Load state…** and **Save state…** list and write state files in the state
directory, which can have folders. They're navigated the same way, and
both dialogs open in the folder last used. Save puts the file in the folder
shown. **New folder** (in Save state…) makes a folder in the one shown and
opens it; a name with a slash, like `talks/morning`, also makes a new folder.

### Options

The **⋯** menu has the control window's View options (window borders,
content labels, test pattern, mullion compensation and so on) and
**Disconnect and forget token**.

## On a phone or tablet

The web UI works in a phone's or tablet's browser, and can be installed as a
home-screen app that opens full screen, with no browser bars.

On an iPhone or iPad:

1. Open **Safari** (it has to be Safari) at `http://wallhost:1910/ui/`
   (`https://` if the wall uses [HTTPS](#https)).
2. Tap **Share** (in Safari's toolbar, or under **⋯** beside the address), then
   **Add to Home Screen**. The app is named after the wall's host —
   `rattler` for `rattler.tacc.utexas.edu` — and can be renamed here.
3. Open it from the home screen and enter the token, with **Remember on this
   device** ticked. Pasting it (from Notes or Messages) saves typing it.

Enter the token in the installed app, not in Safari first: a home-screen app
has its own storage, separate from Safari's. It keeps the token from then on,
unless the app is removed or the wall's token changes. Several walls can
each have their own app.

On Android, Chrome only installs full-screen apps from HTTPS sites: with
[HTTPS](#https) and a trusted certificate, **Install app** works; over plain
HTTP, **Add to Home screen** makes a shortcut that opens in a browser tab.

On narrow screens the side panels become sheets, opened from the bar at the
bottom.

## Streaming a browser tab

The DisplayCluster Streamer extension, in `extension/`, shows a browser tab on
the wall, live: a dashboard, a web visualization, a slide deck. You use the
tab as normal on your own computer, and the wall shows what it shows. It works
in Chrome and Edge (version 116 or later); Firefox and Safari don't have the
tab capture it needs.

It does DesktopStreamer's job for a single tab, with nothing to install but
the extension: it sends the tab to the wall's `/stream` WebSocket, on the same
port and with the same token as the web UI (see [Streaming](#streaming)).

### Installing it

The extension isn't in the Chrome Web Store, so it's loaded from a copy of
the `extension/` folder:

1. Open `chrome://extensions` (in Edge, `edge://extensions`).
2. Turn on **Developer mode**.
3. Click **Load unpacked** and choose the `extension` folder.
4. Pin it to the toolbar: the puzzle-piece button, then the pin beside
   **DisplayCluster Streamer**.
5. Its options page opens from the extension's **⋮** menu → **Options**, or
   from **Set up…** in its popup. Enter the wall's address (a host name, such
   as `rattler.tacc.utexas.edu`, or `https://rattler.tacc.utexas.edu` if the
   wall uses [HTTPS](#https); port 1910 is assumed) and its token, then
   **Save**. Chrome asks to let the extension contact that address, which
   **Test connection** needs; streaming works either way, but with it an
   error can say exactly what went wrong.

Chrome keeps the extension, and the settings, until it's removed. If the
folder is moved or deleted, Chrome disables it; load it again from the new
place.

### Using it

1. In the tab to show, click the extension's button.
2. Change the **Window name** if you like; it starts as the tab's title.
3. Click **Start streaming**. The window opens on the wall, and the button
   shows **ON** for that tab.

**Stop streaming** in the same popup, or closing the tab, closes the window
on the wall. **Alt+Shift+W** starts or stops streaming the current tab without
opening the popup (it can be changed at `chrome://extensions/shortcuts`).
Several tabs can stream at once, each to its own window.

The tab keeps streaming when it's in the background, in another window, or
navigates to another page. Chrome shows its sharing indicator in the tab
while it does.

If it can't stream, the button shows a red **!**, and the popup says why: the
wall couldn't be reached (the address, the network, or its firewall), or the
wall refused the stream (usually the token). If another stream already has
the window name, it's shown as "Name (2)".

### The picture

- A tab is sent at its size on your screen, in screen pixels, up to the
  **Largest size** in the options (3840 by default). For a sharp picture on a
  large part of the wall, make the browser window large, or use a screen with
  more pixels.
- Resizing the window reshapes the picture on the wall, but keeps the number
  of pixels it started with; to get a sharper picture after making the window
  larger, stop and start streaming again.
- Frames are sent only when the page changes, up to the **Frame rate** in the
  options. A still page costs nothing, and the wall keeps showing it.
- Frames are JPEG, at the **Quality** in the options. Video plays, at the
  frame rate. There's no sound.
- While a tab is streaming, the wall stays awake.

## Scripting with DC.py

`python/DC.py` is a Python client for the API. It's a single file that uses
only Python's standard library, so there's nothing to install: copy `DC.py`
next to your script, or put the repo's `python/` directory on `PYTHONPATH`.
Any machine that can reach the wall's port 1910 can run scripts.

```python
from DC import DC

dc = DC('wallhost')                  # or DC() for this machine
print(dc.getConfiguration())         # (tiles wide, tiles high)

name = dc.open('/data/images/mars.jpg', x=0, y=0, w=2, h=2)
dc.reposition(name, 1, -1, -1, -1)   # -1 keeps a value as it is
dc.fill(name)                        # fill the wall...
dc.fill(name, False)                 # ...and put it back
dc.close(name)

dc.showContent()                     # print what's open
```

Two things to know before writing one:

- **Coordinates are in tiles.** `(0, 0)` is the wall's top-left corner and
  `(tilesWide, tilesHigh)`, from `getConfiguration()`, its bottom-right.
  Fractions are fine: `x=1.5` is halfway across the second tile. Mullions
  don't count; a window 1 wide covers exactly one tile.
- **File paths are the wall's.** `open()` takes a path as the wall's
  machines see it, not the machine running the script. Under apptainer that's
  the same as the path on the wall's hosts, for anything in the wall
  account's home directory. `listMedia()` returns paths ready to pass to
  `open()`.

It finds the token in `$DISPLAYCLUSTER_API_TOKEN` or
`~/.displaycluster/api_token` (on the machine it runs on), or take it as
`DC('wallhost', token='...')`. The port is `$DISPLAYCLUSTER_API_PORT` or 1910.

For a wall using [HTTPS](#https), pass `https=True` (or set
`DISPLAYCLUSTER_API_HTTPS=1`). With a self-signed certificate, also give it a
copy of the certificate to trust: `DC('wallhost', cafile='api_cert.pem')`
(or `$DISPLAYCLUSTER_API_CA`), which implies `https=True`.

Windows are named by their file's path unless given a name
(`dc.open(uri, name='mars')`); a file opened twice gets `#2`, `#3`, … on the
end. `open()` returns the name the window got. `dc.content` holds every window
by name: `{uri, x, y, w, h, hidden, filled, sensorMask, showWhenLit, zoom,
centerX, centerY}`,
refreshed after every change or by `dc.updateContent()`.

| Methods | |
|---|---|
| `open`, `close`, `clear`, `reposition`, `rename`, `hide`, `reveal`, `moveToFront`, `fill` | windows |
| `zoom(name, zoom, centerX, centerY)` | the view of a window's content: `zoom` 1 shows all of it, 2 half of it across; the center is in fractions of the content |
| `openDirectory(root, dir, cols, rows)` | a folder's files tiled across the wall |
| `mediaRoots()`, `listMedia(root, dir, sort, descending, offset, limit)` | browsing the media directories |
| `loadState(file)`, `saveState(file)`, `clearState()` | state files, relative to the state directory |
| `makeStateFolder(dir)` | make a folder in the state directory |
| `setSensors(value, mask)`, `getSensors()`, `setSensorControl(name, sensorMask, showWhenLit)` | [sensors](#sensors) |
| `getOptions()`, `setOptions(**options)`, and `setConstrainAspectRatio`, `setShowWindowBorders`, `setShowContentLabels` | display options |
| `sleepWall()`, `wakeWall()`, `status()` | the screensaver, and who has control |
| `takeControl(force)`, `releaseControl()`, `disconnect()` | control, by hand |
| `create_event_list(script)`, `run_events(events)` | play a timed script of windows opening and closing (below) |

Failures are printed and the script carries on; `DC(..., strict=True)` raises
`DCError` instead.

`reposition()` takes `-1` for any of `x`, `y`, `w`, `h` to keep as it is;
`zoom()` keeps any it isn't given.

### Timed scripts

A timed script is a JSON list of windows, each with when it opens and closes,
in seconds from the start:

```json
[
    {"uri": "/data/images/a.png", "open": 0, "close": 10, "x": 0, "y": 0, "w": 2, "h": 2},
    {"uri": "/data/images/b.png", "open": 2, "close": 8,  "x": 2, "y": 0, "w": 2, "h": 2}
]
```

```python
dc.run_events(dc.create_event_list('show.json'))   # or a list instead of a file
```

`run_events` clears the wall first, and returns after the last event.
`examples/run_script.py` plays `examples/script` this way.

### Examples

| File | What it does |
|---|---|
| `examples/run_script.py` | plays the timed script `examples/script`: `python3 run_script.py wallhost` |
| `examples/sensors.py` | sets sensors from the keyboard (`s 4 on`, `t 2`, `a off`) and shows them in red and green; `setup` lays out a red and a green window for each sensor |
| `python/Bounce.ipynb` | a Jupyter notebook that bounces every open window around the wall, then puts them back |
| `python/reposition_test.py` | opens a movie, waits, then moves and resizes it |
| `scripts/cyclestates` | shows saved layouts in turn; [below](#cycling-through-states-cyclestates) |

### Control in scripts

`DC` takes control of the wall the first time it changes something, and gives
it back when the script exits (or at `dc.disconnect()`). Reading needs no
control. Its control lapses after 30 seconds without a request; `DC` takes it
back the next time it changes something, as long as nobody else has taken it
meanwhile.

If someone else has control, the script's changes are refused (and printed as
errors) — so a script doesn't override a person using the wall. `DC(...,
force=True)` takes control regardless. `label=` names the script, which is
what the control window's banner and other clients show while it has control.

A script that makes a change and then waits a long time should give control
back straight away, so it doesn't lock other clients out for the next 30
seconds:

```python
dc = DC(label='cyclestates')

while True:
    for state in ['antarctic.dcx', 'Gulf.dcx', 'webb3.dcx']:
        dc.loadState(state)
        dc.releaseControl()
        sleep(900)
```

### Cycling through states: cyclestates

`scripts/cyclestates` shows a list of saved layouts in turn, for ever, as a
gallery or lobby loop:

```bash
cyclestates -f states.txt                         # from a list, 15 minutes each
cyclestates -t 60 antarctic.dcx Gulf.dcx webb3.dcx
cyclestates -H wallhost -f states.txt             # a wall elsewhere, or over HTTPS
```

| Option | Means |
|---|---|
| `-H host` | the wall's head node; by default `localhost`. Over HTTPS, use the name on its certificate |
| `-t seconds` | how long to show each state; by default 900 |
| `-f listfile` | take the states from a file, one per line; `#` starts a comment. `config/states_stallion.txt` is an example |
| `--force` | take control even if someone else has it |
| `--once` | go through the list once, then stop |

States are named relative to the state directory, as with `loadState()`. The
token and HTTPS come from the same places as for `DC.py`, so over HTTPS set
`DISPLAYCLUSTER_API_HTTPS=1`.

It logs each state and its windows, with the time. It's built to run for days:
- **It lets people use the wall.** It holds control only while loading. If
  someone else has control when a state's turn comes, the state is skipped and
  the wall left alone, and the next one is tried when that time is up. `--force`
  takes control anyway.
- **A missing state doesn't blank the wall.** It's logged and skipped, and what
  was showing stays up for that state's time.
- **It survives the wall stopping.** While the wall is down it says so and tries
  every 30 seconds, so it can start before the wall and carry on after a restart.

Stop it with Ctrl-C, or `pkill -f '[c]yclestates'`. To run it after you log out,
start it in [tmux](setup.md#in-tmux). To run it at set times, see
[On a schedule](setup.md#on-a-schedule).

It runs on any machine that can reach the wall, with Python 3 and `DC.py`, which
it looks for beside itself, in `../python`, and on `PYTHONPATH`. To copy both out
of the image into `~/bin`:

```bash
apptainer exec displaycluster.sif cat /usr/local/bin/cyclestates > ~/bin/cyclestates
apptainer exec displaycluster.sif cat /usr/local/python/DC.py > ~/bin/DC.py
chmod +x ~/bin/cyclestates
```

## Control: one at a time

Only one client can change the wall at a time — a browser, a script, or the
control window.

- A client **takes control**, and then holds it for as long as it's active.
  Control **lapses after 30 seconds** without a request from it (an open web
  UI keeps it alive through its live connection).
- A web UI whose control lapsed — its tab was in the background, or the
  computer slept — takes it back without asking, as long as nobody else has
  taken control meanwhile. It does that when the page is shown again, if it
  was in use within the last 10 minutes (after longer it just shows **Take
  control**), and whenever a change you make is refused because of the lapse.
  Control released, or taken back at the control window, isn't retaken.
- Taking control from someone who has it asks for confirmation in the web UI,
  and takes `force=True` in `DC.py`.
- While a remote client has control, the **control window is read-only** but
  still shows the wall as it changes, with a banner naming who has control and
  a **Take control** button to take it back.
- Watching needs no control: any number of browsers can follow the wall.

## The screensaver

The wall goes to sleep after `DISPLAYCLUSTER_TIMEOUT` seconds without
activity, showing `DISPLAYCLUSTER_SCREENSAVER_IMAGE` bouncing around, and
wakes when someone uses it.

- Changes from anywhere wake the wall and count as activity. Only watching
  doesn't, so an open browser tab doesn't keep the wall awake.
- While a remote client has control, the wall's own idle timer is paused and
  the client decides when it sleeps: the web UI puts it to sleep after the
  same timeout without input. When control ends, the wall's timer starts
  again.
- In the web UI, **Sleep** puts the wall to sleep. While it's asleep, the wall
  view greys out over the layout that will come back; a click or key wakes it.
  Anyone can wake the wall, with or without control.

## Sensors

Windows can be shown and hidden by sensors - light sensors, buttons, anything
that's on or off - reported to the wall as a single number, the **sensor
value**: bit *i* is set when sensor *i* is on ("lit"). Whatever reads the
sensors sends the value with `PUT /sensors`, and the wall updates every window
under sensor control at once, on the same frame, however many there are.

A window is under sensor control when its **`sensorMask`** isn't 0. Bit *i*
of the mask makes it follow sensor *i*, and **`showWhenLit`** says how:

- `showWhenLit: true` - shown when any of its sensors is lit, hidden otherwise
- `showWhenLit: false` - shown when none of its sensors is lit

So a red window and a green one stacked in the same place, both with mask
`4` (sensor 2) and `showWhenLit` `false` and `true`, show green while sensor 2
is lit and red while it isn't.

- Windows start out with `sensorMask` 0, and sensors leave them alone.
- A window put under sensor control, opened with a mask, or loaded from a
  state file follows the current value straight away.
- Hiding or showing a sensor-controlled window by hand lasts until the
  sensors next change. Setting its mask to 0 leaves it as it is.
- State files keep each window's `sensorMask` and `showWhenLit` (as
  `<sensorMask>` and `<showWhenLit>`, written only for windows that have a
  mask). The sensor value itself isn't saved; it starts at 0.
- Setting the sensors needs the token but **not control**, so whatever reports
  them doesn't lock people out of arranging the wall, or get locked out by
  them. It doesn't wake the wall or keep it awake either: asleep, the value is
  kept, and the windows follow it when the wall wakes.
- `PUT /sensors` with a `mask` changes only those bits, so several sources can
  each report their own sensors without overwriting the others'.

In `DC.py`:

```python
dc.setSensorControl('b1s0-green', 0b000100)                     # sensor 2, shown when lit
dc.setSensorControl('b1s0-red', 0b000100, showWhenLit = False)  # shown when it isn't
dc.setSensors(0b000100)                                         # sensor 2 lit, the rest not
dc.setSensors(0b01, mask = 0b11)                                # just sensors 0 and 1
```

`getSensorCount()` says how many sensors the wall offers.

`examples/sensors.py` tries it all from the keyboard: `setup` puts a red
window and a green one, stacked, in a cell for each sensor, in columns of two
(sensor *n* in column *n* ÷ 2, on top for even *n*), and then `s 4 on`, `t 2`
(toggle) and `a off` (all) set them, with the result shown in the terminal in
the same layout:

```
$ python3 examples/sensors.py wallhost
  [  0 off ]  [  2 on  ]  [  4 on  ]
  [  1 off ]  [  3 off ]  [  5 off ]
  value 0x14
sensors> t 2
```

In the web UI, the window inspector has a checkbox per sensor and a
**Lit / Dark** choice (set `DISPLAYCLUSTER_SENSORS` to the number of sensors),
and the top bar shows the sensor value.

## API reference

Every request carries the token, as `Authorization: Bearer <token>`. (A
browser's `EventSource` and `WebSocket` can't send headers, so `/events` and
`/stream` also accept it as `?access_token=`.) Changes also carry the control lease, as
`X-DC-Lease: <lease>`. Bodies and replies are JSON; errors come back with a
4xx status and `{"error": "..."}`.

Windows are named in URLs percent-encoded (`/` as `%2F`, `#` as `%23`).
Coordinates are in tile units: `(0, 0)` is the wall's top-left corner and
`(tilesWide, tilesHigh)` its bottom-right.

### Status and control

| Request | Body | Returns / does |
|---|---|---|
| `GET /status` | | `{asleep, idleTimeout, controller}`; `controller` is `{id, label}` or null |
| `GET /events` | `?lease=` optionally | server-sent events: `{status, windows, options, sensors}`, now and whenever they change (at most ten times a second) |
| `POST /control` | `{label?, force?}` | take control: `{lease, id, label, timeout}`; `423` naming the holder if someone has it and `force` isn't set |
| `POST /control/heartbeat` | | keep control while idle |
| `DELETE /control` | | give control back |
| `POST /sleep` | | start the screensaver (needs control) |
| `POST /wake` | | end it (needs no control) |

### The wall and its windows

| Request | Body | Returns / does |
|---|---|---|
| `GET /config` | | size in tiles and pixels, screen and mullion sizes, `sensorCount`, version |
| `GET /windows` | | every window, back to front (while asleep, the layout waking will restore) |
| `POST /windows` | `{uri, name?, x?, y?, w?, h?, sensorMask?, showWhenLit?}` | open a file |
| `POST /windows/directory` | `{root, dir?, cols?, rows?}` | open a folder's files tiled across the wall; without `cols`/`rows`, a grid that fits them (up to 36) |
| `DELETE /windows` | | close every window |
| `GET /windows/{name}` | | one window |
| `PATCH /windows/{name}` | any of `{x, y, w, h, hidden, front, zoom, centerX, centerY, name, filled, sensorMask, showWhenLit}` | change a window |
| `DELETE /windows/{name}` | | close a window |
| `GET /windows/{name}/thumbnail` | | a small JPEG of its content |

A window is:

```json
{"name": "mars", "uri": "/data/images/mars.jpg",
 "x": 0.0, "y": 0.0, "w": 2.0, "h": 1.5, "z": 3,
 "hidden": false, "filled": false,
 "sensorMask": 0, "showWhenLit": true,
 "zoom": 1.0, "centerX": 0.5, "centerY": 0.5,
 "contentWidth": 4096, "contentHeight": 3072,
 "thumbnail": "/windows/mars/thumbnail?v=1727541234000"}
```

`z` is its place in the stacking order, from the back. `zoom`, `centerX` and
`centerY` are the view of its content: `zoom` 1 shows all of it, and the
center is in content fractions. `filled: true` makes a window as large as
fits, centered and in front, remembering where it was; `false` puts it back.
`sensorMask` and `showWhenLit` are its [sensor control](#sensors).
`thumbnail` is null for content that has none (image pyramids, streams, and
images too big to decode cheaply); its URL changes when the file does.

### Sensors

| Request | Body | Returns / does |
|---|---|---|
| `GET /sensors` | | `{value}` |
| `PUT /sensors` | `{value, mask?}` | set the sensor value, or just the bits in `mask`; returns `{value}` (needs no control) |

### Options and state

| Request | Body | Returns / does |
|---|---|---|
| `GET /options`, `PATCH /options` | any of `constrainAspectRatio`, `showWindowBorders`, `showContentLabels`, `showTestPattern`, `enableMullionCompensation`, `showZoomContext`, `enableStreamingSynchronization`, `showStreamingSegments`, `showStreamingStatistics`, as booleans | the display options |
| `GET /state?dir=` | | a folder of the state directory (default its top): subfolders by name, `{type: "directory", name, dir, modified}`, then state files newest first, `{type: "state", name, file, modified}`. `dir` and `file` are relative to the state directory |
| `POST /state/load` | `{file}` | load a state file |
| `POST /state/save` | `{file}` | save one (`.dcx` is added if missing); folders in `file` are made as needed |
| `POST /state/folder` | `{dir}` | make a folder in the state directory, and any above it; `{dir, existed}`. Like saving, it needs no control |

### Browsing

| Request | Returns |
|---|---|
| `GET /media` | the media directories: `[{name, path}]` |
| `GET /media/{root}?dir=&sort=&order=&offset=&limit=` | a page of a directory: `{root, dir, parent, total, offset, entries}` |

`sort` is `name`, `modified` or `size`, `order` `asc` or `desc`, and pages are
up to 1000 entries (200 by default). Folders come first. Each entry has
`name`, `type` (`directory`, `image`, `movie`, `svg` or `pyramid`) and
`modified`, and either `dir` (to list next) or `uri` and `size` (to open).
Only files the wall can open are listed, unless `all=true`, which lists the
rest too, with `type` `other` and no `uri`.

### Streaming

`/stream/{name}` is a WebSocket that puts a live picture on the wall, as
DesktopStreamer does, for senders that can't use DesktopStreamer's own
protocol, such as a browser. Each binary message is one whole frame, a JPEG of
any size. The first frame opens a window named `{name}`, later ones replace
the picture, and the window closes when the connection does.

- The server answers each frame with a text message, `ack`. Waiting for it
  before sending the next frame keeps frames from queueing up.
- Send frames only when the picture changes. An open connection keeps the wall
  awake, frames or not: it wakes the wall when it opens and counts as activity
  until it closes.
- Names are cut to 63 bytes. A second connection with the name of a stream
  already on the wall is closed with code 1008; a frame that isn't a JPEG
  closes the connection with code 1007.
- Streaming needs no control: a stream adds its own window and changes no one
  else's.

In Python, with the `websocket-client` package:

```python
import websocket

ws = websocket.create_connection("ws://wallhost:1910/stream/My%20screen?access_token=" + TOKEN)
for jpeg in frames:
    ws.send_binary(jpeg)
    ws.recv()           # "ack"
ws.close()              # the window closes
```

### Needing control

`POST`, `PATCH` and `DELETE` on `/windows…` and `/options`, and
`POST /state/load` and `/sleep`, need control. Everything else doesn't.

Without it they're refused with `423` and `{error, controller, lapsed}`:
`controller` is whoever has control, or `null`, and `lapsed` is true when the
request's lease is one that timed out and no one has taken control since, so
its client can take control again without displacing anyone. A lease ended by
`DELETE /control` or by the control window's Take control doesn't count as
lapsed.

### With curl

```bash
TOKEN=...
LEASE=$(curl -s -H "Authorization: Bearer $TOKEN" -X POST http://wallhost:1910/control \
        -d '{"label": "curl"}' | python3 -c "import json,sys; print(json.load(sys.stdin)['lease'])")
curl -H "Authorization: Bearer $TOKEN" -H "X-DC-Lease: $LEASE" -X POST http://wallhost:1910/windows \
     -d '{"uri": "/data/images/mars.jpg", "name": "mars", "x": 0, "y": 0, "w": 2, "h": 2}'
curl -H "Authorization: Bearer $TOKEN" -H "X-DC-Lease: $LEASE" -X DELETE http://wallhost:1910/control
```

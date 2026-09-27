import os
import json
import socket
import atexit
import urllib.request
import urllib.error
import urllib.parse
from time import sleep

# Client for DisplayCluster's remote-control HTTP API (see the README's
# "Remote control API" section). Windows are identified by name - by default
# the path of the file they show, with #2, #3, ... appended when that file is
# already open. Coordinates are in tile units.

class DCError(Exception):
    def __init__(self, status, message):
        super().__init__('%s (HTTP %d)' % (message, status))
        self.status = status
        self.message = message

def _read_token():
    token = os.environ.get('DISPLAYCLUSTER_API_TOKEN')
    if token:
        return token
    path = os.path.expanduser('~/.displaycluster/api_token')
    if os.path.isfile(path):
        with open(path) as f:
            return f.read().strip()
    return None

class DC:
    # token: defaults to $DISPLAYCLUSTER_API_TOKEN, then ~/.displaycluster/api_token
    # strict: raise DCError on failures, rather than printing them and carrying on
    # label: how this client appears to others while it controls the wall
    # force: take control even if someone else has it
    #
    # Only one client at a time may change the wall. DC takes control the first
    # time it changes something (and again if its control lapsed from 30 s of
    # quiet), and gives it back at close() or exit; reading needs no control.
    def __init__(self, host = 'localhost', port = None, nx = 1, ny = 1, token = None, strict = False,
                 label = None, force = False):
        self.host = host
        self.port = port if port else int(os.environ.get('DISPLAYCLUSTER_API_PORT', 1910))
        self.nx   = nx
        self.ny   = ny
        self.token = token if token else _read_token()
        self.strict = strict
        self.label = label if label else 'DC.py on %s (pid %d)' % (socket.gethostname(), os.getpid())
        self.force = force
        self.lease = None
        atexit.register(self.disconnect)
        self.updateContent()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.disconnect()

    def _send(self, method, path, body):
        url = 'http://%s:%d%s' % (self.host, self.port, path)
        data = json.dumps(body).encode() if body is not None else None
        req = urllib.request.Request(url, data = data, method = method)
        if data is not None:
            req.add_header('Content-Type', 'application/json')
        if self.token:
            req.add_header('Authorization', 'Bearer ' + self.token)
        if self.lease:
            req.add_header('X-DC-Lease', self.lease)
        try:
            with urllib.request.urlopen(req) as response:
                return response.status, json.loads(response.read())
        except urllib.error.HTTPError as e:
            try:
                reply = json.loads(e.read())
            except Exception:
                reply = { 'error': e.reason }
            return e.code, reply

    def _fail(self, status, reply):
        message = reply.get('error', 'HTTP %d' % status) if isinstance(reply, dict) else 'HTTP %d' % status
        if self.strict:
            raise DCError(status, message)
        print('error:', message)
        return None

    # change: this request changes the wall, so needs control
    def _request(self, method, path, body = None, change = False):
        if change and not self.lease and not self.takeControl():
            return None
        status, reply = self._send(method, path, body)
        if status == 423 and change and reply.get('controller') is None:
            # our control lapsed and nobody else has it: take it back and retry
            self.lease = None
            if not self.takeControl():
                return None
            status, reply = self._send(method, path, body)
        if status >= 400:
            return self._fail(status, reply)
        return reply

    # returns True if this client now controls the wall
    def takeControl(self, force = None):
        status, reply = self._send('POST', '/control', { 'label': self.label, 'force': self.force if force is None else force })
        if status != 200:
            self._fail(status, reply)
            return False
        self.lease = reply['lease']
        return True

    def releaseControl(self):
        if self.lease:
            self._send('DELETE', '/control', None)
            self.lease = None

    # gives up control, if held; also done automatically at exit
    def disconnect(self):
        try:
            self.releaseControl()
        except Exception:
            pass

    # {asleep, idleTimeout, controller}
    def status(self):
        return self._request('GET', '/status')

    def _window_path(self, name):
        return '/windows/' + urllib.parse.quote(name, safe = '')

    def updateContent(self):
        windows = self._request('GET', '/windows')
        self.content = {}
        for w in windows or []:
            self.content[w['name']] = { 'uri': w['uri'], 'x': w['x'], 'y': w['y'], 'w': w['w'], 'h': w['h'], 'hidden': w['hidden'] }

    def getConfiguration(self):
        config = self._request('GET', '/config')
        return config['tilesWide'], config['tilesHigh']

    # any of x, y, w, h given as -1 keeps its current value
    def reposition(self, name, x, y, w, h):
        params = { k: v for k, v in (('x', x), ('y', y), ('w', w), ('h', h)) if v != -1 }
        self._request('PATCH', self._window_path(name), params, change = True)
        self.updateContent()

    # returns the new window's name - the file's path unless name is given (or
    # the file is already open, in which case #2, #3, ... is appended)
    def open(self, uri, x = 0, y = 0, w = 1, h = 1, name = None):
        params = { 'uri': uri, 'x': x, 'y': y, 'w': w, 'h': h }
        if name:
            params['name'] = name
        window = self._request('POST', '/windows', params, change = True)
        self.updateContent()
        return window['name'] if window else None

    def rename(self, name, new_name):
        self._request('PATCH', self._window_path(name), { 'name': new_name }, change = True)
        self.updateContent()

    def hide(self, name):
        self._request('PATCH', self._window_path(name), { 'hidden': True }, change = True)
        self.updateContent()

    def reveal(self, name):
        self._request('PATCH', self._window_path(name), { 'hidden': False }, change = True)
        self.updateContent()

    def moveToFront(self, name):
        self._request('PATCH', self._window_path(name), { 'front': True }, change = True)
        self.updateContent()

    def setConstrainAspectRatio(self, onOff):
        self._request('PATCH', '/options', { 'constrainAspectRatio': bool(onOff) }, change = True)
        self.updateContent()

    def setShowWindowBorders(self, onOff):
        self._request('PATCH', '/options', { 'showWindowBorders': bool(onOff) }, change = True)
        self.updateContent()

    def setShowContentLabels(self, onOff):
        self._request('PATCH', '/options', { 'showContentLabels': bool(onOff) }, change = True)
        self.updateContent()

    def close(self, name):
        self._request('DELETE', self._window_path(name), change = True)
        self.updateContent()

    def clear(self):
        self._request('DELETE', '/windows', change = True)
        self.updateContent()

    def showContent(self):
        print("Current contents")
        for name, props in self.content.items():
            hidden_str = ' [hidden]' if props['hidden'] else ''
            uri_str = '' if props['uri'] == name else ' (' + props['uri'] + ')'
            print(name + uri_str, props['x'], props['y'], props['w'], props['h'], hidden_str)

    def clearState(self):
        self.clear()

    # state files are relative to the wall's state directory
    def loadState(self, state):
        self._request('POST', '/state/load', { 'file': state }, change = True)
        self.updateContent()

    def saveState(self, state):
        self._request('POST', '/state/save', { 'file': state })

    # any of the display options /options reports, e.g. setOptions(showTestPattern = True)
    def setOptions(self, **options):
        return self._request('PATCH', '/options', options, change = True)

    def getOptions(self):
        return self._request('GET', '/options')

    def sleepWall(self):
        self._request('POST', '/sleep', change = True)

    # anyone may wake the wall; it needs no control
    def wakeWall(self):
        self._request('POST', '/wake')

    # the media roots the wall browses: [{name, path}]
    def mediaRoots(self):
        return self._request('GET', '/media') or []

    # lists a directory under a media root (the first, by default): a list of
    # entries with 'name', 'type' ('directory', 'image', 'movie', 'svg' or
    # 'pyramid'), 'modified', and either 'dir' (to pass back here) or 'uri' and
    # 'size' (to pass to open). sort is 'name', 'modified' or 'size'.
    def listMedia(self, root = None, dir = '', sort = 'name', descending = False, offset = 0, limit = 200):
        if root is None:
            roots = self.mediaRoots()
            if not roots:
                return []
            root = roots[0]['name']
        query = urllib.parse.urlencode({ 'dir': dir, 'sort': sort, 'order': 'desc' if descending else 'asc',
                                         'offset': offset, 'limit': limit })
        result = self._request('GET', '/media/' + urllib.parse.quote(root, safe = '') + '?' + query)
        return result['entries'] if result else []

    # opens the files in a media directory tiled across the wall - a grid of
    # cols x rows if given, otherwise one that fits them; returns how many opened
    def openDirectory(self, root, dir = '', cols = None, rows = None):
        params = { 'root': root, 'dir': dir }
        if cols and rows:
            params['cols'] = cols
            params['rows'] = rows
        result = self._request('POST', '/windows/directory', params, change = True)
        self.updateContent()
        return result['opened'] if result else 0

    def create_event_list(self, script):
        if isinstance(script, str):
          with open(script) as f:
              j = json.load(f)
        elif isinstance(script, list):
              j = script
        events = []
        for content in j:
            events.append(["open", content['open'], content])
            events.append(["close", content['close'], content])
        return sorted(events, key=lambda a: a[1])

    def run_events(self, event_list):
        self.clear()
        now = 0
        # the name each event's window got when it opened, so its close event
        # closes that window even if the same file is open more than once
        names = {}
        for e in event_list:
            if now < e[1]:
                delay = e[1] - now
                sleep(delay)
                now = now + delay
            if e[0] == "open":
                names[id(e[2])] = self.open(e[2]['uri'], e[2]['x'], e[2]['y'], e[2]['w'], e[2]['h'])
            elif names.get(id(e[2])):
                self.close(names[id(e[2])])

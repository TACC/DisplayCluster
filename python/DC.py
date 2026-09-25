import os
import json
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
    def __init__(self, host = 'localhost', port = None, nx = 1, ny = 1, token = None, strict = False):
        self.host = host
        self.port = port if port else int(os.environ.get('DISPLAYCLUSTER_API_PORT', 1910))
        self.nx   = nx
        self.ny   = ny
        self.token = token if token else _read_token()
        self.strict = strict
        self.updateContent()

    def _request(self, method, path, body = None):
        url = 'http://%s:%d%s' % (self.host, self.port, path)
        data = json.dumps(body).encode() if body is not None else None
        req = urllib.request.Request(url, data = data, method = method)
        if data is not None:
            req.add_header('Content-Type', 'application/json')
        if self.token:
            req.add_header('Authorization', 'Bearer ' + self.token)
        try:
            with urllib.request.urlopen(req) as response:
                return json.loads(response.read())
        except urllib.error.HTTPError as e:
            try:
                message = json.loads(e.read())['error']
            except Exception:
                message = e.reason
            if self.strict:
                raise DCError(e.code, message)
            print('error:', message)
            return None

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
        self._request('PATCH', self._window_path(name), params)
        self.updateContent()

    # returns the new window's name - the file's path unless name is given (or
    # the file is already open, in which case #2, #3, ... is appended)
    def open(self, uri, x = 0, y = 0, w = 1, h = 1, name = None):
        params = { 'uri': uri, 'x': x, 'y': y, 'w': w, 'h': h }
        if name:
            params['name'] = name
        window = self._request('POST', '/windows', params)
        self.updateContent()
        return window['name'] if window else None

    def rename(self, name, new_name):
        self._request('PATCH', self._window_path(name), { 'name': new_name })
        self.updateContent()

    def hide(self, name):
        self._request('PATCH', self._window_path(name), { 'hidden': True })
        self.updateContent()

    def reveal(self, name):
        self._request('PATCH', self._window_path(name), { 'hidden': False })
        self.updateContent()

    def moveToFront(self, name):
        self._request('PATCH', self._window_path(name), { 'front': True })
        self.updateContent()

    def setConstrainAspectRatio(self, onOff):
        self._request('PATCH', '/options', { 'constrainAspectRatio': bool(onOff) })
        self.updateContent()

    def setShowWindowBorders(self, onOff):
        self._request('PATCH', '/options', { 'showWindowBorders': bool(onOff) })
        self.updateContent()

    def setShowContentLabels(self, onOff):
        self._request('PATCH', '/options', { 'showContentLabels': bool(onOff) })
        self.updateContent()

    def close(self, name):
        self._request('DELETE', self._window_path(name))
        self.updateContent()

    def clear(self):
        self._request('DELETE', '/windows')
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
        self._request('POST', '/state/load', { 'file': state })
        self.updateContent()

    def saveState(self, state):
        self._request('POST', '/state/save', { 'file': state })

    # lists a directory under the wall's media directory: a list of entries
    # with 'name', 'type' ('directory' or 'file'), and either 'dir' (to pass
    # back here) or 'uri' (to pass to open)
    def listMedia(self, dir = ''):
        result = self._request('GET', '/media?dir=' + urllib.parse.quote(dir))
        return result['entries'] if result else []

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

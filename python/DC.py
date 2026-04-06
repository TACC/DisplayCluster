import sys
import socket
import json
from time import sleep

last_msg = "none"

class Connection:

    def __init__(self, k, host = None):
        if host:
            self.skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.skt.connect((host, k))
        else:
            self.skt = k

    def Receive(self):
        b = self.skt.recv(4)
        sz = int.from_bytes(b, 'little')
        buf = b''
        while sz  > 0:
            b = self.skt.recv(sz)
            buf += b
            sz -= len(b)
        msg = buf.decode();
        last_msg = msg
        return json.loads(msg);

    def Send(self, j):
        msg = json.dumps(j).encode('ascii')
        sz = len(msg)
        b = sz.to_bytes(4, 'little')
        self.skt.send(b)
        self.skt.send(msg)
        
class Server:

    def __init__(self, port): 
        self.skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.skt.bind(('localhost', port))
        self.skt.listen(5) 

    def Accept(self):
        skt = self.skt.accept()
        return Connection(skt)

class DC:
    def __init__(self, host = 'localhost', port = 1910, nx = 1, ny = 1):
        self.host = host
        self.port = port
        self.nx   = nx
        self.ny   = ny
        self.updateContent()

    def updateContent(self):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "update" })
        j_in = c.Receive()
        self.content = {}
        for i in j_in:
            # i = [x, y, w, h, uri, hidden]
            self.content[i[4]] = { 'x': i[0], 'y': i[1], 'w': i[2], 'h': i[3], 'hidden': bool(i[5]) }

    def getConfiguration(self):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "get configuration" })
        j_in = c.Receive()
        return j_in[0], j_in[1]

    def reposition(self, uri, x, y, w, h):
        if uri not in self.content.keys():
            print('uri ', uri, ' not open')
            return
        content = self.content[uri]
        if x == -1: x = content['x']
        if y == -1: y = content['y']
        if w == -1: w = content['w']
        if h == -1: h = content['h']
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "reposition", "uri": uri, "x": x, "y": y, "w": w, "h": h} )
        self.updateContent()

    def open(self, uri, x = 0, y = 0, w = 1, h = 1):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "open", "uri": uri, "x": x, "y": y, "w": w, "h": h} )
        self.updateContent()

    def hide(self, uri):
        if uri not in self.content:
            print('uri ', uri, ' not open')
            return
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "hide", "uri": uri })
        self.updateContent()

    def reveal(self, uri):
        if uri not in self.content:
            print('uri ', uri, ' not open')
            return
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "reveal", "uri": uri })
        self.updateContent()

    def moveToFront(self, uri):
        if uri not in self.content:
            print('uri ', uri, ' not open')
            return
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "top", "uri": uri })
        self.updateContent()

    def setConstrainAspectRatio(self, onOff):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "constrain aspect ratio", "state": onOff})
        self.updateContent()

    def setShowWindowBorders(self, onOff):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "show window borders", "state": onOff})
        self.updateContent()

    def setShowContentLabels(self, onOff):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "show content labels", "state": "on" if onOff else "off"})
        self.updateContent()

    def close(self, uri):
        c = Connection(self.port, self.host)
        c.Send({'cmd': 'close', 'uri': uri })
        self.updateContent()

    def clear(self):
        c = Connection(self.port, self.host)
        c.Send({'cmd': 'clear' })
        self.updateContent()

    def showContent(self):
        print("Current contents")
        for uri, props in self.content.items():
            hidden_str = ' [hidden]' if props['hidden'] else ''
            print(uri, props['x'], props['y'], props['w'], props['h'], hidden_str)

    def clearState(self):
        c = Connection(self.port, self.host)
        c.Send({'cmd': 'clear state'})
        self.updateContent()

    def loadState(self, state):
        c = Connection(self.port, self.host)
        c.Send({'cmd': 'load state', 'state': state})
        self.updateContent()
        
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
        for e in event_list:
            if now < e[1]:
                delay = e[1] - now
                sleep(delay)
                now = now + delay
            if e[0] == "open":
                self.open(e[2]['uri'], e[2]['x'], e[2]['y'], e[2]['w'], e[2]['h'])
            else:
                self.close(e[2]['uri'])
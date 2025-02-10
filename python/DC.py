import sys
import socket
import json
from time import sleep

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
        b = self.skt.recv(sz)
        #print("Received ", sz, " bytes: ", b)
        msg = b.decode();
        return json.loads(msg);

    def Send(self, j):
        msg = json.dumps(j).encode('ascii')
        sz = len(msg)
        b = sz.to_bytes(4, 'little')
        #print("Sending ", sz, " bytes: ", msg)
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
    def __init__(self, host, port, nx = 1, ny = 1):
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
            self.content[i[4]] = i[:4]

    def reposition(self, uri, x, y, w, h):
        if uri not in self.content.keys():
            print('uri ', uri, ' not open')
            return
        content = self.content[uri]
        if x == -1: x = content[0]
        if y == -1: y = content[1]
        if w == -1: w = content[2]
        if h == -1: h = content[3]
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "reposition", "uri": uri, "x": x, "y": y, "w": w, "h": h} )
        self.updateContent()

    def open(self, uri, x = 0, y = 0, w = 1, h = 1):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "open", "uri": uri, "x": x, "y": y, "w": w, "h": h} )
        self.updateContent()

    def setConstrainAspectRatio(self, onOff):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "constrain aspect ratio", "state": onOff})
        self.updateContent()

    def setShowWindowBorders(self, onOff):
        c = Connection(self.port, self.host)
        c.Send({ "cmd": "show window borders", "state": onOff})
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
        for c in self.content:
            print(c, self.content[c])

    def create_event_list(self, script):
        events = []
        for content in script:
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
            

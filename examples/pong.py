import os
import time
from pydc import *

c = pyContent(os.environ['DISPLAYCLUSTER_DIR'] + "/examples/pong.jpg")
cw = pyContentWindowManager(c)
dg = pyDisplayGroupPython()
dg.addContentWindowManager(cw)

def pong(seconds=30.0, sleepInterval=1.0/30.0, dx=1.0/500.0, dy=1.0/500.0):

    startTime = time.time()

    while time.time() - startTime < seconds:
        c = cw.getCoordinates()

        x = c[0]
        y = c[1]
        w = c[2]
        h = c[3]

        x = x + dx

        if (x+w) > 1.0:
            x = 2.0 - (x+w) - w
            dx = -dx
        elif x < 0.0:
            x = -x
            dx = -dx

        y = y + dy

        if (y+h) > 1.0:
            y = 2.0 - (y+h) - h
            dy = -dy
        elif y < 0.0:
            y = -y
            dy = -dy

        cw.setPosition(x,y)

        time.sleep(sleepInterval)

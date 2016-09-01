import os
from time import time
from time import sleep

dgm = pydc.pyDisplayGroupPython()

orig = []
deltas = []

if 'DISPLAYCLUSTER_SCREENSAVER_IMAGE' in os.environ:
	pongtime = int(os.environ['DISPLAYCLUSTER_SCREENSAVER_PONGTIME'])
else:
	pongtime = 0

for i in range(dgm.getNumContentWindowManagers()):
	cw = dgm.getPyContentWindowManager(i)
	x,y,w,h = cw.getCoordinates()
	deltas.append([x,y,w,h,dx,dy])
	orig.append(deltas[-1][:2])
	cw.setPosition(1,1)

dx = 1.0/500.0
dy = 1.0/500.0

pongimage = None
if 'DISPLAYCLUSTER_SCREENSAVER_IMAGE' in os.environ:
	fname = os.environ['DISPLAYCLUSTER_SCREENSAVER_IMAGE']
	if fname[0] != '/':
		fname = os.environ['DISPLAYCLUSTER_DIR'] + '/' + fname
	if os.path.isfile(fname):
		tt = pydc.pyContent(fname)
		pongimage = pydc.pyContentWindowManager(pydc.pyContent(fname))
		dgm.addContentWindowManager(pongimage)

t0 = time()
while pydc.pyMyPythonQt().get_idle():
	if (pongimage != None) and ((time() - t0) < pongtime):
		x,y,w,h = pongimage.getCoordinates()
		x = x + dx
		if (x + w) > 1.0: 
			x = 2.0 - (x+w) - w
			dx = -dx
		elif x < 0.0:
			x = -x
			dx = -dx
			
		y = y + dy
		if (y + h) > 1.0: 
			y = 2.0 - (y+h) - h
			dy = -dy
		elif y < 0.0:
			y = -y
			dy = -dy

		pongimage.setPosition(x, y)
	else:
		if pongimage != None:
			dgm.removeContentWindowManager(pongimage)
			pongimage = None
	sleep(0.03333)

if pongimage != None:
	dgm.removeContentWindowManager(pongimage)
	pongimage = None

for i in range(dgm.getNumContentWindowManagers()):
	cw = dgm.getPyContentWindowManager(i)
	cw.setPosition(orig[i][0], orig[i][1])

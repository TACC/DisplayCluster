print "hello!"
from time import sleep

dgm = pydc.pyDisplayGroupPython()

sleepInterval = 1.0/30.0
dx = 1.0/500.0
dy = 1.0/500.0

orig = []
deltas = []

print 'aaaa'

for i in range(dgm.getNumContentWindowManagers()):
	# print 'bbbb'
	cw = dgm.getPyContentWindowManager(i)
	# print 'cccc'
	x,y,w,h = cw.getCoordinates()
	deltas.append([x,y,w,h,dx,dy])
	# print 'dddd'
	orig.append(deltas[-1][:2])
	# print 'eeee'

print "ffff"

while pydc.pyMyPythonQt().get_idle():
	print "sleep"

	for i in range(dgm.getNumContentWindowManagers()):
		cw = dgm.getPyContentWindowManager(i)
		c = deltas[i]

		x  = c[0]
		y  = c[1]
		w  = c[2]
		h  = c[3]
		dx = c[4]
		dy = c[5]

		print x, y, w, h

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

		print x, y, w, h

		c[0] = x
		c[1] = y
		c[4] = dx
		c[5] = dy

		cw.setPosition(x,y)

	print "sleep"
	sleep(0.02)

for i in range(dgm.getNumContentWindowManagers()):
	cw = dgm.getPyContentWindowManager(i)
	cw.setPosition(orig[i][0], orig[i][1])

print "bye!"

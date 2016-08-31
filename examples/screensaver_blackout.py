print "hello!"
from time import sleep

dgm = pydc.pyDisplayGroupPython()

dgm.pushState()

while dgm.getNumContentWindowManagers() > 0:
	cw = dgm.getPyContentWindowManager(0)
	cwp = cw.getPyContent()
	print(cwp.getURI())
	dgm.removeContentWindowManager(cw)

while pydc.pyMyPythonQt().get_idle():
	print "sleep"
	sleep(2)

dgm.popState()
print "bye!"

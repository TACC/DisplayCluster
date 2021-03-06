import cherrypy
import os
# import simplejson
import sys
from socket import *

MEDIA_DIR = os.path.join(os.getenv("DISPLAYCLUSTER_DIR"), u"remote/media")
DISPLAYCLUSTER_PORT = os.getenv("DISPLAYCLUSTER_PORT")
DISPLAYCLUSTER_CHERRYPY_PORT = os.getenv("DISPLAYCLUSTER_CHERRYPY_PORT")

if DISPLAYCLUSTER_CHERRYPY_PORT is None:
	raise OSError('Environment variable DISPLAYCLUSTER_CHERRYPY_PORT is not set')

if DISPLAYCLUSTER_PORT is None:
	raise OSError('Environment variable DISPLAYCLUSTER_PORT is not set')

class Remote(object):
		@cherrypy.expose
		def index(self):
				f = open(os.path.join(MEDIA_DIR, u'top'))
				html = f.read()
				f.close()
				if os.path.isfile(os.path.join(MEDIA_DIR, u'contents')):
					contents = open(os.path.join(MEDIA_DIR, u'contents'))
					for content in contents:
						content = content.strip()
						html = html + "<input type=button value='%s' onclick=foo('%s')>" % (content.split('/')[-1], content)
						html = html + "<br>\n"
					contents.close()
				f = open(os.path.join(MEDIA_DIR, u'bottom'))
				html = html + f.read()
				f.close()
				return html

		@cherrypy.expose
		def select(self, name):
			s = socket(AF_INET, SOCK_STREAM)
			s.connect(("localhost", int(DISPLAYCLUSTER_PORT)))
			s.send(name)
			s.close()
			# cherrypy.response.headers['Content-Type'] = 'application/json'
			# return simplejson.dumps(dict(response="OK"))
			return "OK"

config = {'/media':
	{
		'tools.staticdir.on': True,
		'tools.staticdir.dir': MEDIA_DIR,
	}
}

cherrypy.config.update({'server.socket_host': '0.0.0.0'})
cherrypy.tree.mount(Remote(), '/', config=config)
cherrypy.config.update({'server.socket_port': int(DISPLAYCLUSTER_CHERRYPY_PORT)})
cherrypy.engine.start()

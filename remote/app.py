import cherrypy
import os
# import simplejson
import sys
from socket import *

MEDIA_DIR = os.path.join(os.getenv("DISPLAYCLUSTER_DIR"), u"remote/media")

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
			s.connect(("localhost", 1900))
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
cherrypy.engine.start()

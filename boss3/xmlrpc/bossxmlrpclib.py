#
# This module is a list of modifications done to the xmlrpclib module.
# xmlrpclib can be upgraded at any time, or removed and the dist python
# version can be used. Our changes to xmlrpclib will stay intact as they
# are here. The idea and some of the code for this module came from the
# pyxmlrpclib module that comes with py-xmlrpc.
#
# Current modifications to xmlrpclib are as follows:
#  gzip support - If the client sends an "Accept-Encoding: gzip" html
#		  header to the server
#                 it may reply with a Content-Encoding: gzip header in
#		  the reply meaning that
#                 the body of the html reply is gziped.
#  py-xmlrpc	- If the py-xmlrpc module is installed
#		  (http://sourceforge.net/projects/py-xmlrpc) there is a
#		  speed increase of about 5 times (more depending on the
#		  size of the data being sent over xml.
#
# Written by Simon Coggins (soahc@chaos.id.au)



# Import everything from xmlrpclib
import xmlrpclib
from xmlrpclib import *
from StringIO import StringIO
import types, sys
from boss3.util import bosslog

log = bosslog.getLogger()

# Try importing _xmlrpc if we fail, use xmlrpclib.
try:
	import _xmlrpc
except:
	_xmlrpc = None


# If we have the fast module. Lets override the modules we can speed up.
# This way we still look like the xmlrpclib module, but faster!
if _xmlrpc is not None and hasattr(_xmlrpc, "parseCall"):
	Boolean	= _xmlrpc.boolean
	xmlrpclib.Boolean = Boolean
	True = Boolean(1)
	False = Boolean(0)
	xmlrpclib.True = True
	xmlrpclib.False = False
	DateTime = _xmlrpc.dateTime
	Binary = _xmlrpc.base64

	class Marshaller:
			def __init__(self, Encoding=None, allow_none=0):
					pass

			def dumps(self, values):
				log.debug("funcs", "Marshaller.dumps() Super Fast XMLer!")
				if ((isinstance(values, Fault))
				or  (isinstance(values, Fault))):
						d = { 'faultString' : values.faultString,
								  'faultCode' : values.faultCode }
						return ('<fault>\n%s</fault>\n' % _xmlrpc.encode(d))
				else:
						l = ['<params>\n']
						for item in values:
								l.append('\t<param>\n\t\t')
								l.append(_xmlrpc.encode(item))
								l.append('\n\t</param>\n')
						l.append('</params>\n')
						data = string.join(l, '')
						# Hack cause _xmlrpc doesn't do built in boolean right.
						data = data.replace("<int>False</int>", "<boolean>0</boolean>")
						data = data.replace("<int>True</int>", "<boolean>1</boolean>")
						return data

	xmlrpclib.FastMarshaller = Marshaller


	# override the Marshaller class with a (much) faster version
	class Parser:
			def __init__(self, target):
					self.result = None
					self.target = target
					self.file = StringIO()

			def feed(self, s):
					self.file.write(s)

			def close(self):
					v = self.file.getvalue()
					self.target.set_data(v)

	# ensure that we use our fast parser
	#
	xmlrpclib.FastParser = Parser

	# override the Marshaller class with a (much) faster version
	#
	class Unmarshaller:
			def __init__(self, *args):
				self.data = None
				self.method = None
				self.value = None

			def set_data(self, data):
				self.data = data
					
			def close(self):
				log.debug("funcs", "Unmarshaller.close() Super fast XML!")
				s = string.lstrip(self.data)
				if s[:7] == '<value>':
						self.value = _xmlrpc.decode(data)
				elif ((s[:19] == "<?xml version='1.0'")
				or    (s[:19] == '<?xml version="1.0"')):
						s = string.lstrip(s[s.index("?>")+3:])
						if s[:16] == '<methodResponse>':
								try:
										s = ("HTTP/1.0 200 OK\r\n"
											 "Content-length: %d\r\n\r\n"
											 "%s" % (len(self.data),self.data))
										self.value = (_xmlrpc.parseResponse(s)[0], )
								except _xmlrpc.fault:
										v = sys.exc_value
										raise Fault(v.faultCode, v.faultString)
						elif s[:12] == '<methodCall>':
								(self.method, self.value) = _xmlrpc.parseCall(self.data)
				if self.value == None:
						raise TypeError, "unrecognized data: %.40s..." % s

				return self.value

			def getmethodname(self):
				return self.method
				
	xmlrpclib.FastUnmarshaller = Unmarshaller            
#
# Here we have some replacements to standard classes to enable gziped xml support.
#

class Transport(Transport):
	def request(self, host, handler, request_body, verbose=0):
		# issue XML-RPC request

		h = self.make_connection(host)
		if verbose:
			h.set_debuglevel(1)

		self.send_request(h, handler, request_body)
		self.send_host(h, host)
		self.send_user_agent(h)
		self.send_content(h, request_body)

		errcode, errmsg, headers = h.getreply()

		if errcode != 200:
			raise ProtocolError(
				host + handler,
				errcode, errmsg,
				headers
				)

		self.verbose = verbose

		try:
			sock = h._conn.sock
		except AttributeError:
			sock = None

		return self._parse_response(h.getfile(), sock, headers=headers)

	def send_content(self, connection, request_body):
		connection.putheader("Content-Type", "text/xml")
		connection.putheader("Content-Length", str(len(request_body)))
		connection.putheader("Accept-Encoding", "gzip")
		connection.endheaders()
		if request_body:
			connection.send(request_body)

	def _parse_response(self, file, sock, headers=0):
		# read response from input file/socket, and parse it

		p, u = self.getparser()
		gzip=0
		clength = 1024
		cencoding = 0

		if headers:
			clength = headers.getheader("Content-Length")
			cencoding = headers.getheader("Content-Encoding")


		if cencoding and cencoding.find("gzip") != -1:
			log.debug("xmlrpc", "Server sent reply in gzip format!")
			gzip = 1

		try:
			clength = int(clength)
		except:
			clength = 1024

		response = ""
		while 1:
			if sock:
				data = sock.recv(clength)
			else:
				data = file.read(clength)
			if not data:
				break
			response += data
			if self.verbose:
				print "body:", repr(data)

		if gzip:
			response = zlib.decompress(response)

		p.feed(response)
		file.close()
		p.close()

		return u.close()


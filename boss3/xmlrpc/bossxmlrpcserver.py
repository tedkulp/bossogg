# bossxmlrpcserver is a modified version of SimpleXMLRPCServer. Changes
# to the base xml server module are done here to make updating the base
# module easier. We also use this module to add a threaded server to handle
# multiple requests at once.
#
# Written by Simon Coggins (soahc@chaos.id.au)

import SimpleXMLRPCServer
from SimpleXMLRPCServer import *
import threading
import bossxmlrpclib
import traceback
from boss3.util import bosslog

log = bosslog.getLogger()

# The dynamic loader is a weird thing.
try:
    zlib
except:
    import zlib

# Overide the default xmlrpclib that SimpleXMLRPCServer imports with
# our custom version
SimpleXMLRPCServer.xmlrpclib = bossxmlrpclib

# Add gzip support to the handler
class SimpleXMLRPCRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_POST(self):
		try:
			data = self.rfile.read(int(self.headers["content-length"]))
			response = self.server._marshaled_dispatch(data, getattr(self, '_dispatch', None))
		except: # This should only happen if the module is buggy
			# internal error, report as HTTP server error
			log.exception("Got exception calling xmlrpc marsheller")
			log.error("BAD XML CODE:\n%s", data)
			self.send_response(500)
			self.end_headers()
		else:
			# got a valid XML RPC response
			self.send_response(200)

			# If the response is more than 1k lets compress it if the client
			# supports it
			if len(response) > 1024 and self.headers.has_key("accept-encoding") and self.headers["accept-encoding"].find("gzip") != -1:
				log.debug("xmlrpc", "Client indicated it can handle GZIP. Lets do it")
				response = zlib.compress(response, 6)
				self.send_header("Content-Encoding", "gzip")

			self.send_header("Content-type", "text/xml")
			self.send_header("Content-length", str(len(response)))
			self.end_headers()
			self.wfile.write(response)

			# shut down the connection
			self.wfile.flush()
			self.connection.shutdown(1)

	def log_request(self, code='-', size='-'):
		"""Selectively log an accepted request."""

		if self.server.logRequests:
			BaseHTTPServer.BaseHTTPRequestHandler.log_request(self, code, size)

# Add threading supprt to the XML Server so we arn't blocking when other
# requests take along time. We still block on large sql operations tho.
class SimpleXMLRPCServer(SocketServer.ThreadingTCPServer, SimpleXMLRPCDispatcher):
	"""Simple XML-RPC server.

	Simple XML-RPC server that allows functions and a single instance
	to be installed to handle requests. The default implementation
	attempts to dispatch XML-RPC calls to the functions or instance
	installed in the server. Override the _dispatch method inhereted
	from SimpleXMLRPCDispatcher to change this behavior.
	"""

	def __init__(self, addr, requestHandler=SimpleXMLRPCRequestHandler,
				 logRequests=1):
		self.logRequests = logRequests
		# Reuse address so we don't have to wait for timeout
		self.allow_reuse_address = 1

		SimpleXMLRPCDispatcher.__init__(self)
		SocketServer.ThreadingTCPServer.__init__(self, addr, requestHandler)


#!/usr/bin/env python

import socket, time, xmlrpclib, string, zlib

class _Method:
    # some magic to bind an XML-RPC method to an RPC server.
    # supports "nested" methods (e.g. examples.getStateName)
    def __init__(self, send, name):
        self.__send = send
        self.__name = name
    def __getattr__(self, name):
        return _Method(self.__send, "%s.%s" % (self.__name, name))
    def __call__(self, *args):
        return self.__send(self.__name, args)

class ServerProxy:
	s = None

	def __init__(self):
		self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	def __getattr__(self, name):
		return _Method(self.sendFunction,name)
	
	def connect(self,hostname='',port=4068):
		if self.s is not None:
			self.s.connect((hostname, port))
	
	def close(self):
		if self.s is not None:
			self.s.close()
		
	def sendFunction(self, fncname, params):
		if self.s is not None:
			xml = string.strip(xmlrpclib.dumps(params,methodname=fncname))
			xml = zlib.compress(xml)
			self.s.send("%i:%s" % (len(xml),xml))
			start = 1
			maxlen = 1
			result = ''
			while len(result) < maxlen or start == 1:
				if start == 1:
					(maxlen,result) = string.split(self.s.recv(1024),':',1)
					maxlen = int(maxlen)
					start = 0
				else:
					result += self.s.recv(1024)
			result = zlib.decompress(result)
			print ("result: %s" % str(xmlrpclib.loads(result)[0][0]))
			time.sleep(.05)
	

server = ServerProxy()
server.connect()
server.info("status")
server.list("artists")
server.list("albums", 2)
server.close()

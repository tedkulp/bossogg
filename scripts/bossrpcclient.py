#!/usr/bin/env python

import socket, time, xmlrpclib, string, zlib, threading

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
	lock = None

	def __init__(self):
		self.lock = threading.Lock()
		self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	def __getattr__(self, name):
		return _Method(self.sendFunction,name)
	
	def connect(self,hostname='',port=4068):
		if self.s is not None:
			self.lock.acquire()
			self.s.connect((hostname, port))
			self.lock.release()
	
	def close(self):
		if self.s is not None:
			self.lock.acquire()
			self.s.close()
			self.lock.release()
		
	def sendFunction(self, fncname, params):
		if self.s is not None:
			xml = string.strip(xmlrpclib.dumps(params,methodname=fncname))
			xml = zlib.compress(xml)
			self.lock.acquire()
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
			self.lock.release()
			result = zlib.decompress(result)
			#print result
			result = xmlrpclib.loads(result)[0][0]
			#print ("result: %s" % str(result))
			time.sleep(.1)
			return result
	

if __name__ == '__main__':
	server = ServerProxy()
	server.connect()
	print server.info("status")
	print server.list("artists")
	print server.list("albums", 2)
	print server.list("songinfo")
	server.close()

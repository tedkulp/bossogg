#!/usr/bin/env python

import socket, select, string, thread, threading
from boss3.util.Session import *
from boss3.xmlrpc import xmlrpclib
from boss3.util import bosslog
from boss3.xmlrpc.XmlRpcServer import XmlRpcInterface
import zlib

log = bosslog.getLogger()

class BossClientInterface:

	threadid = None

	def __init__(self):
		try:
			log.debug("bossrpc","Starting up the bossrpc server")
			self.threadid = BossRpcServer()
			self.threadid.start()
			session = Session()
			session['haveaninterface'] = 1
		except Exception:
			session = Session()
			session['haveaninterface'] = 0
	
	def shutdown(self):
		log.debug("bossrpc","Shutting down bossrpc server")
		self.threadid.shutdown = True
		self.threadid.join()

class BossRpcServer(threading.Thread):

	compression = False

	interface = None

	buflength = 1024

	readable_in = []
	writable_in = []
	errorable_in = []

	lengths = {}
	data = {}
	sessions = {}

	shutdown = False

	def encodeBossRpc(self,obj,methodname=None,methodresponse=False):
		xml = string.strip(xmlrpclib.dumps(obj, methodname=methodname, methodresponse=methodresponse))
		if self.compression == True:
			xml = zlib.compress(xml)
			return "%i:z:%s" % (len(xml),xml)
		else:
			return "%i::%s" % (len(xml),xml)

	def __init__(self):
		threading.Thread.__init__(self)
		try:
			self.serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			self.serversocket.bind(('',4068))
			self.serversocket.setblocking(0)
			self.serversocket.listen(5)

			self.readable_in.append(self.serversocket)

			self.interface = XmlRpcInterface()

		except Exception:
			log.error("Boss-RPC port already bound.  Shutting it down.")
			self.shutdown = True

	def run(self):
		#log = bosslog.getLogger()
		session = Session()
		while self.shutdown == False:
			readable, writable, errorable = select.select(self.readable_in,self.writable_in,self.errorable_in,5.0)
			for i in readable:
				if i == self.serversocket:
					(conn, addr) = i.accept()
					log.debug("bossrpc","Accepted a new connection from: %s" % str(addr[0]))
					self.readable_in.append(conn)
					self.writable_in.append(conn)
				else:
					recvbuf = i.recv(self.buflength)
					if len(recvbuf) == 0:
						log.debug("bossrpc","Removing connection")
						if i.fileno() in self.sessions.keys():
							if (session.hasKey('cmdint')):
								cmdint = session['cmdint']
								cmdint.auth.logout(self.sessions[i.fileno()])
							del self.sessions[i.fileno()]
							log.debug("bossrpc","Removed session")
						else:
							log.debug("bossrpc","No session found to remove")
						i.close()
						self.readable_in.remove(i)
						self.writable_in.remove(i)
					else:
						recvbuf = string.strip(recvbuf)
						if len(recvbuf) > 0:
							if i.fileno() in self.lengths:
								self.data[i.fileno()] += recvbuf
								#log.debug("bossrpc","more data")
							else:
								(msglength,flags,msg) = string.split(recvbuf, ':', 2)
								self.lengths[i.fileno()] = int(msglength)
								self.data[i.fileno()] = msg
								#log.debug("bossrpc","start of data")
						log.debug("bossrpc", "%i - %i" % (self.lengths[i.fileno()], len(self.data[i.fileno()])))
						if self.lengths[i.fileno()] <= len(self.data[i.fileno()]):
							xml = self.data[i.fileno()]
							xml = string.strip(zlib.decompress(xml))
							response = xmlrpclib.loads(xml)
							log.debug("bossrpc","Received xmlrpc message: %s",response)
							if response[1] == "auth" and len(response) == 3 and "login" in response[0]:
								log.debug("bossrpc","Received auth message: %s-%s",str(response[1]),str(response[0]))
								if (session.hasKey('cmdint')):
									cmdint = session['cmdint']
									sessionid = cmdint.auth.login(response[0][1],response[0][2])
									newxml = ""
									if sessionid != None:
										log.debug("bossrpc","Received sessionid: %s",sessionid)
										self.sessions[i.fileno()] = sessionid
										newxml = self.encodeBossRpc(sessionid,methodresponse=True)
									else:
										log.deug("bossrpc","Invalid login")
										newxml = self.encodeBossRpc("Permission Denied",methodresponse=True)
									i.sendall(newxml)
							elif response[1] in dir(self.interface) and callable(getattr(self.interface, response[1])):
								ans = (getattr(self.interface,response[1])(*response[0]),)
								#print ans
								#xml = string.strip(xmlrpclib.dumps(ans, methodresponse=True))
								#log.debug("bossrpc","Sending xmlrpc message: %s",str(ans))
								#xml = zlib.compress(xml)
								#print xml
								bossxml = self.encodeBossRpc(ans,methodresponse=True)
								i.sendall(bossxml)
							del self.lengths[i.fileno()]
							del self.data[i.fileno()]
		readable, writable, errorable = select.select(self.readable_in,self.writable_in,self.errorable_in,0)
		for i in writable:
			i.shutdown()
			i.close()

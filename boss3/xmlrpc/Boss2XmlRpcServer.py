#Boss Ogg - A Music Server
#(c)2003 by Ted Kulp (wishy@comcast.net)
#This project's homepage is: http://bossogg.wishy.org
#
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

from boss3.xmlrpc import SimpleXMLRPCServer
import thread
import socket
import operator
from boss3.util import Logger
from boss3.util.Session import *

class BossClientInterface:

	theserver = None
	threadid = None

	def start_server(self):
		self.theserver.serve_forever()

	def __init__(self):
		#try:
			self.theserver = SimpleXMLRPCServer.SimpleXMLRPCServer(('',4096))
			self.theserver.logRequests = False
			self.theserver.allow_reuse_address = True
			#self.theserver.register_function(self.controlobj.handleRequest, "control")
			self.theserver.register_instance(Boss2XmlRpcInterface())
			self.theserver.register_introspection_functions()
			#self.start_server()
			self.threadid = thread.start_new_thread(self.start_server, ())
			session = Session()
			session['haveaninterface'] = 1
		#except Exception:
		#	session = Session()
		#	session['haveaninterface'] = 0

	def shutdown(self):
		if self.theserver:
			Logger.log("Shutting down boss2 xmlrpc server")
			del self.theserver

class Boss2XmlRpcInterface:

	def __init__(self):
		self.session = Session()
		self.cmd = self.session['cmdint'] 

	def listartists(self):
		return self.cmd.list.artists()
		
	def listalbums(self, artistid):
		return self.cmd.list.albums(artistid)
	
	def listsongs(self, albumid):
		return self.cmd.list.songs(albumid=albumid)

# vim:ts=8 sw=8 noet

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

from boss3.xmlrpc import bossxmlrpcserver
from boss3.xmlrpc import Control
from boss3.xmlrpc import List
from boss3.xmlrpc import Info
from boss3.xmlrpc import Auth
from boss3.xmlrpc import DB
from boss3.xmlrpc import Util
from boss3.xmlrpc import Load
from boss3.xmlrpc import Set
from boss3.xmlrpc import Edit
import thread
import socket
import operator
from boss3.util import Logger
from boss3.util.Session import *
from boss3.util import bosslog

log = bosslog.getLogger()

class BossClientInterface:

	theserver = None
	threadid = None

	def start_server(self):
		if self.theserver != None:
			while 1:
				#self.theserver.serve_forever()
				self.theserver.handle_request()

	def __init__(self):
		try:
			self.theserver = bossxmlrpcserver.SimpleXMLRPCServer(('',4069))
			self.theserver.logRequests = False
			self.theserver.allow_reuse_address = True
			#self.theserver.register_function(self.controlobj.handleRequest, "control")
			self.theserver.register_instance(XmlRpcInterface())
			self.theserver.register_introspection_functions()
			#self.start_server()
			self.threadid = thread.start_new_thread(self.start_server, ())
			session = Session()
			session['haveaninterface'] = 1
		except Exception:
			session = Session()
			session['haveaninterface'] = 0

	def shutdown(self):
		if self.theserver:
			log.debug("xmlrpc", "Shutting down xmlrpc server")
			self.theserver.server_close()
			del self.theserver

class XmlRpcInterface:

	controlobj = None
	listobj = None
	authobj = None
	dbobj = None
	infoobj = None
	utilobj = None
	loadobj = None
	setobj = None
	editobj = None

	def __init__(self):
		self.controlobj = Control.Control()
		self.listobj = List.List()
		self.authobj = Auth.Auth()
		self.dbobj = DB.DB()
		self.infoobj = Info.Info()
		self.utilobj = Util.Util()
		self.loadobj = Load.Load()
		self.setobj = Set.Set()
		self.editobj = Edit.Edit()

	def auth(self, params, *args): 
		return self.authobj.handleRequest(params, args)

	def control(self, params, *args):
		return self.controlobj.handleRequest(params, args)
	
	def list(self, params, *args):
		return self.listobj.handleRequest(params, args)

	def db(self, params, *args):
		return self.dbobj.handleRequest(params, args)

	def info(self, params, *args):
		return self.infoobj.handleRequest(params, args)

	def util(self, params, *args):
		return self.utilobj.handleRequest(params, args)

	def load(self, params, *args):
		return self.loadobj.handleRequest(params, args)

	def set(self, params, *args):
		return self.setobj.handleRequest(params, args)
	
	def edit(self, params, *args):
		return self.editobj.handleRequest(params, args)

# vim:ts=8 sw=8 noet

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

from util.Session import *
from util import Logger
from util import bosslog
import md5
import time

log = bosslog.getLogger()

class UserManager:

	users = None
	dbh = None

	def __init__(self, dbh):
		session = Session()
		session['usrmgr'] = self
		self.users = {}
		self.dbh = dbh

	def _cleanup(self):
		for someuser in self.users.keys():
			if self.users[someuser].lastdate < (time.time() - (60 * 30)) and self.users[someuser].type == "stateless":
				del self.users[someuser]

	def listUsers(self):
		self._cleanup()
		result = []
		for someuser in self.users.keys():
			usertoadd = {}
			usertoadd['username'] = self.users[someuser].username
			usertoadd['minconnected'] = (time.time() - self.users[someuser].startdate) / 60
			result.append(usertoadd)
		return result

	def authUser(self, username, password):
		self._cleanup()
		log.debug("auth","authing user: %s" % username)
		result = self.dbh.authUser(username, password)
		if (result != None):
			log.debug("auth","Found userid: %s" % result['userid'])
			newuser = User(result['userid'])
			log.debug("auth","User Created")
			newuser.authlevel = result['authlevel']
			log.debug("auth","Set Auth Level: %s" % newuser.authlevel)
			newuser.username = username
			log.debug("auth","Set Username: %s" % newuser.username)
			self.users[newuser.sessionid] = newuser
			log.debug("auth","User added to list")
			return newuser.sessionid
		else:
			log.debug("auth","User not found")
			return None

	def touchSession(self, sessionid):
		self._cleanup()
		if sessionid in self.users:
			self.users[sessionid].lastdate = time.time()
			return 1
		else:
			return 0

	def cancelSession(self, sessionid):
		self._cleanup()
		if sessionid in self.users:
			del self.users[sessionid]
			return 1
		else:
			return 0

	def getSessionIdFromFileno(self, fileno):
		self._cleanup()
		for i in self.users.keys():
			if self.users[i].fileno == fileno:
				return i
		return None

	def getUserIdFromSession(self, sessionid):
		self._cleanup()
		if sessionid in self.users:
			return self.users[sessionid].uid
		else:
			return None

	def getAuthLevelFromSession(self, sessionid):
		self._cleanup()
		if sessionid in self.users:
			return self.users[sessionid].authlevel
		else:
			return None

class User:

	uid = -1
	sessionid = None
	startdate = None
	lastdate = None
	username = ""
	type = "stateless"
	fileno = -1
	
	def __init__(self, uid):
		m = md5.new()
		m.update(str(uid))
		m.update(str(time.time()))
		self.sessionid = m.hexdigest()
		self.uid = uid
		self.startdate = time.time()
		self.lastdate = time.time()

# vim:ts=8 sw=8 noet

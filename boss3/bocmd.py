#Boss Ogg - A Music Server
#(c)2003 by David Salib, Ted Kulp (wishy@comcast.net)
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

import cmd
from util.Session import *
import xmlrpc.bossxmlrpclib as xmlrpclib
import util.UTFstring
import string
import getpass
import time

class Bocmd(cmd.Cmd):
	"""
	Boss ogg command line interface
	to add a command follow this pattern : 
	def do_bub(self,args):
		do bub
	def do_xmlrpc_bub(self,args):
		do bub xmlrpc style
	def help_bub(self):
		print "Bub bubbud dubbed."

	if the command is modeless ie there is only one way to do it whether
	you use xmlrpc or not. Only code the do_bub method and add this line
	to the do_bub method:
		modeless("bub")
	"""

	sessionid = None
	server = None
	session = None
	tags = None
	prompt = ">> "
	remote = True
	modelessCommands = ["mode","exit","quit","setmode","connect","help","?","EOF"]
	hostname = "default : localhost"
	
	#import CommandInterface

	#def modeless(self,methodname):
		#self.modelessCommands[:0] = [methodname]
	#def do_xmlrpc_bub(self,args):
	#	bub = None
	#	bub = self.server.list("queue",1,10)
	#	print bub

	#def do_mode(self,args):
	#	if self.remote:
	#		print "remote"
	#	else:
	#		print "local"
	#def do_setmode(self, args):
	#	if args == "remote":
	#		self.remote = True
	#		print "Mode: remote server\nServer: " + self.hostname
	#	elif args == "local":
	#		self.remote = False
	#		print "Mode: local function call"
	#	else:
	#		print "invalide argument use: remote xor local"
	#def help_setmode(self):
	#	print "Use remote to use XMLrpc to communicate with the server\nor use local to use python function calls."
	#	print "Usage: setMode remote || local"

	def do_list(self,args):
		cmdargs = string.split(args," ",1)
		cmd = cmdargs[0]

		if (cmd == "artists"):
			artists = None
			cmdargs = string.split(args," ",1)
			if (len(cmdargs)==2):
				artists = self.server.list("artists",str(cmdargs[1]))
			else:
				artists = self.server.list("artists")
			if artists:
				artists = util.UTFstring.decode(artists)
				for a in artists:
						print '%4d %s' % (a['artistid'], util.UTFstring.decode(a['artistname']))
		elif (cmd == "queue"):
			queue = None
			cmdargs = string.split(args," ",2)
			if (len(cmdargs)==3):
				queue = self.server.list("queue",int(cmdargs[1]),int(cmdargs[2]))
			elif (len(cmdargs)==2):
				queue = self.server.list("queue",int(cmdargs[1]))
			else:
				queue = self.server.list("queue")
			if queue:
				queue = util.UTFstring.decode(queue)
				if len(cmdargs) > 1 and cmdargs[1] == "1":
					print "bub"
					for q in queue:
# start of an ugly block
						print "Index: %4d		Songid: %4d		Track #: %2d \n\
Artist: %s\n\
Album: %s\n\
Song: %s\n\
Filename %-70s	%d\n\
Percentage played: %5.1d	Times played: %s" % (q['index'], q['songid'], q['tracknum'], util.UTFstring.decode(q['artistname']), util.UTFstring.decode(q['albumname']), util.UTFstring.decode(q['songname']), util.UTFstring.decode(q['filename']), q['filesize'], q['percentagecompleted'], util.UTFstring.decode(q['timesplayed']))
# end of ugly block
				else:
					for q in queue:
						print "Index: %4d Songid: %4d" % (q['index'], q['songid'])
			else:
				print "oops, that didn't work"
		elif (cmd == "albums"):
			albums = None
			cmdargs = string.split(args," ",2)
			if (len(cmdargs)==3):
				albums = self.server.list("albums",int(cmdargs[1]),str(cmdargs[2]))
			elif (len(cmdargs)==2):
				albums = self.server.list("albums",int(cmdargs[1]))
			else:
				print "Error in given parameters"
			if albums:
				albums = util.UTFstring.decode(albums)
				for a in albums:
					print '%4d %s' % (a['albumid'], util.UTFstring.decode(a['albumname']))
			else:
				print "There was an error"
		elif (cmd == "songs"):
			cmdargs = string.split(args," ",3)
			if (len(cmdargs)>2):
				type = cmdargs[1]
				if (type == "artistid" or type == "albumid"):
					songs = None
					if (len(cmdargs)==4):
						songs = self.server.list("songs",type,int(cmdargs[2]),str(cmdargs[3]))
					elif (len(cmdargs)==3):
						songs = self.server.list("songs",type,int(cmdargs[2]))
					if songs:
						songs = util.UTFstring.decode(songs)
						for s in songs:
							print '%4d %s %s' % (s['songid'], util.UTFstring.decode(s['albumname']), util.UTFstring.decode(s['songname']))
				else:
					print "%s is not a valid type" % (type)
			else:
				print "No artistid or albumid given"
		elif (cmd == "users"):
			users = self.server.list("users")
			for u in users:
				print '%s %5.2d' % (u['username'], u['minconnected'])
		elif (cmd == "top"):
			cmdargs = string.split(args," ",2)
			if (len(cmdargs)>1):
				type = cmdargs[1]
				if (type == "artists" or type == "albums" or type == "songs"):
					songs = None
					if (len(cmdargs)==3):
						songs = self.server.list("top",type,int(cmdargs[2]))
					elif (len(cmdargs)==2):
						songs = self.server.list("top",type)
					if songs:
						if type == "artists":
							songs = util.UTFstring.decode(songs)
							for s in songs:
								print '%2d %s' % (s['count'], util.UTFstring.decode(s['artistname']))
						elif type == "albums":
							songs = util.UTFstring.decode(songs)
							for s in songs:
								print '%2d %s %s' % (s['count'], util.UTFstring.decode(s['artistname']), util.UTFstring.decode(s['albumname']))
						elif type == "songs":
							songs = util.UTFstring.decode(songs)
							for s in songs:
								print '%2s %s %s %s' % (s['count'], util.UTFstring.decode(s['artistname']), util.UTFstring.decode(s['albumname']), util.UTFstring.decode(s['songname']))
				else:
					print "%s is not a valid type" % (type)
			else:
				print "Top what? artists, albums or songs."
		elif (cmd == "songinfo"):
			cmdargs = string.split(args," ",1)
			songinfo = None
			if len(args) == 2:
				songid = cmdargs[1]
				songinfo = self.server.list("songinfo",int(songid))
			else:
				songinfo = self.server.list("songinfo")
			if songinfo:
				s = util.UTFstring.decode(songinfo)
# start of an ugly block
				print "Songid: %4d			Track #: %2d \n\
Artist: %s\n\
Album: %s\n\
Song: %s\n\
Filename %-70s	%d\n\
Percentage played: %5.1d	Times played: %s" % (s['songid'], s['tracknum'], util.UTFstring.decode(s['artistname']), util.UTFstring.decode(s['albumname']), util.UTFstring.decode(s['songname']), util.UTFstring.decode(s['filename']), s['filesize'], s['percentagecompleted'], util.UTFstring.decode(s['timesplayed']))
# end of ugly block
		else:
			print "Valid options are artists, albums, songs, users, songinfo and top."
	
	def help_list(self):
		print "List commands: artists | albums | songs | users | songinfo | top"

	def do_info(self, args):
		cmdargs = string.split(args," ",1)
		cmd = cmdargs[0]
		status = None
		status = self.server.info("status")
		if status:
			for sn, si in status.items():
				print "%-20s: %s" % (sn,str(si))

	def help_info(self):
		print "Info commands: status"

	def do_load(self, args):
		cmdargs = string.split(args," ",2)
		#cmd = cmdargs[0] + "id"
		cmd = cmdargs[0]
		#if len(cmdargs) >= 2 and cmd == "artistid" or cmd == "albumid" or cmd == "songid":
		if len(cmdargs) >= 2 and cmd == "artist" or cmd == "album" or cmd == "song":
			if len(cmdargs) == 2:
				if self.server.load(cmd,int(cmdargs[1])):
					print cmd + " added"
				else:
					print "There was an error"
			elif len(cmdargs) == 3:
				if self.server.load(cmd,int(cmdargs[1]),cmdargs[2]):
					print cmd + " added"
				else:
					print "There was an error"
		elif cmd == "move" and len(cmdargs) == 3:
				if self.server.load("move",int(cmdargs[1]),int(cmdargs[2])):
					print "moved"
				else:
					print "There was an error"
		elif cmd == "remove" and len(cmdargs) == 2:
				if self.server.load("remove",int(cmdargs[1])):
					print "removed"
				else:
					print "There was an error"
		elif cmd == "shuffle":
				if self.server.load("shuffle"):
					print "shuffled"
				else:
					print "There was an error"
		else:
			print "invalid arguments: use artist | album | song | move | remove | shuffle"

	def help_load(self):
		print "Load commands: artist | album | song | move | remove"

	def do_control(self, args):
		cmdargs = string.split(args," ",1)
		cmd = cmdargs[0]
		if cmd == "pause" or cmd == "stop" or cmd == "play" or cmd == "next" or cmd == "playindex":
			if cmd == "playindex":
				if len(cmdargs) == 2:
					self.server.playindex(cmdargs[1])
				else:
					print "No indexid given"	
			elif self.server.control(cmd):
				print cmd
			else:
				print "There was an error."
		else:
			print "Usage: control play | stop | pause | next | playindex"
	def help_control(self):
		print "Control commands: play, stop, pause, next"

	def do_set(self, args):
		if args == "shutdown":
			if self.server.set("shutdown"):
				print "server has shutdown"
			else:
				print "There was an error"
		else:
			print "Usage: set shutdown"

	def help_set(self):
		print "Set commands: shutdown"
			
	def do_rip(self, args):
		cmdargs = string.split(args," ",1)
		cmd = cmdargs[0]
		if cmd == "getCDDB":
			self.tags = self.server.db(cmd,"auto")
			print "Found the following tags in the CDDB:"
			for i in self.tags:
				print i
		elif args == "pyrip":
			filenames = ["01.ogg", "02.ogg", "03.ogg", "04.ogg", "05.ogg", "06.ogg", "07.ogg", "08.ogg", "09.ogg", "10.ogg", "11.ogg"]
			self.server.db(cmd,self.tags,0,"auto")
			ret = self.server.db("pyrip_update")
			while ret['done'] != 1:
				print ret
				time.sleep (2)
				ret = self.server.db("pyrip_update")
			print ret
		elif args == "pyrip_update":
			print self.server.db("pyrip_update")
		else:
			print "Usage: rip [getCDDB | pyrip | pyrip_update]"

	def do_connect(self, args):
		self.server = None
		hostname = "127.0.0.1"
		portnum = "4069"
		cmdargs = string.split(string.rstrip(args))
		if (len(cmdargs) > 0):
			hostname = cmdargs[0]
		if (len(cmdargs) > 1):
			portnum = cmdargs[1]
		hostnamestring = "http://%s:%s" % (hostname, portnum)
		username = "blah"
		password = "blah"
#		if (len(cmdargs) == 2):
#			username = cmdargs[0]
#			hostnamestring = cmdargs[1]
#			password = getpass.getpass("Enter password for " + username + ": ")
#		else:
#			hostnamestring = cmdargs[0]
#			username = "blah"
#			password = "blah"
#		if not hostnamestring:
#			hostnamestring = "http://localhost:4069"
		try:
			print "Attempting to connect to server at: " + hostnamestring
			self.server = xmlrpclib.Server(hostnamestring)
			stuff = self.server.util("version")
			#print stuff
			if stuff is not None:
				print "Connected to server"
				self.sessionid = "stuff"
		except:
			print "Could not connect to server" 
			return
		#else:
			#return server
		#try:
		#	self.sessionid=self.server.auth("login",username,password)
			#print "Session Id: %s" % self.sessionid
		#except:
		#	pass
			#print "Session Id Error"

	def help_connect(self):
		print "Connect to a Boss Ogg server"
		print "Usage: connect [host] [port] (defaults to 127.0.0.1:4069)"

	def do_exit(self,args):
		return 1

	def help_exit(self):
		print "Usage: exit"

	def do_quit(self,args):
		return 1

	def help_quit(self):
		print "Usage: quit"

	def do_EOF(self, args):
		print
		return 1
	
	def pcarseline(self, line):
		"""Parseline which supports unicode
		"""
		line = line.strip()
		if not line:
			return None, None, line
		elif line[0] == '?':
			line = 'help ' + line[1:]
		elif line[0] == '!':
			if hasattr(self, 'do_shell'):
				line = 'shell ' + line[1:]
			else:
				return None, None, line
		i, n = 0, len(line)
		while i < n and line[i] not in " ": i +=1
		cmd, arg = line[:i], line[i:].strip()
		return cmd, arg, line

	def onecmd(self, line):
		"""Interpret the argument as though it had been typed in response
		to the prompt.

		This has been overridden to provide support for the remote and local
		call mode.

		The return value is a flag indicating whether interpretation of
		commands by the interpreter should stop.

		"""
		cmd, arg, line = self.parseline(line)
		if not line:
			return self.emptyline()
		if cmd is None:
			return self.default(line)
		self.lastcmd = line
		if cmd == '':
			return self.default(line)
		else:
			func = None
			try:
				if cmd in self.modelessCommands or not self.remote:
					func = getattr(self, 'do_' + cmd)
				else:
					if self.sessionid:
						func = getattr(self, 'do_' + cmd)
					#elif  not self.sessionid:
						# TODO make do_connect return an error code and check it.
					#	self.do_connect("http://localhost:4069")
					#	func = getattr(self, 'do_' + cmd)
					else:
						#print "Connect to a server using connect hostname or change mode."
						print "Not connected to a server.  Please use the connect command to connect to a running server."
			except AttributeError:
				return self.default(line)
			if func is not None:
				return func(arg)
			else:
				return None

# vim:ts=8 sw=8 noet

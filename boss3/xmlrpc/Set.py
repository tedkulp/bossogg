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

from boss3.util import Logger
from boss3.util.Session import *
import boss3.xmlrpc.bossxmlrpclib as xmlrpclib

class Set:
	"""
	Set realm.  Used to set various server level settings
	and controls.  These commands are geared toward admin
	type users who control the very running aspects of the
	server itself.


	util("shutdown"): Schedule the server to cleanly shut
	itself down immediately.  This will shutdown all interfaces,
	save the current queue and other runtime information to the
	database and then end the running process.

	Parameters:
	* None

	Returns:
	* int (always 1)


	util("endofqueue"): Sets the endofqueue action for the server.
	This will take effect the next time endofqueue is called.

	Parameters:
	* endofqueueaction - string
	* endofqueueparam - string

	Returns:
	* int (always 1)


	util("saveconfig"): Saves the config file back to the
	filesystem.  This will overwrite the existing config file.

	Parameters:
	* None

	Returns:
	* int (always !)


	util("volume"): Sets the playback volume.

	Parameters:
	* volume - int

	Returns:
	* int (always 1)
	"""

	def handleRequest(self, cmd, argstuple):

		session = Session()

		args = [] 
		for i in argstuple:
			args.append(i)

		#print "args " + str(args)
		#print "len args " + str(len(args))

		if (session.hasKey('cmdint')):
			cmdint = session['cmdint']

			if (cmd == "shutdown"):
				return cmdint.set.shutdown()
			elif (cmd == "saveconfig"):
				return cmdint.set.saveconfig()
			elif (cmd == "endofqueue"):
				if len(args) == 1:
					return cmdint.set.endofqueue(args[0])
				elif len(args) == 2:
					return cmdint.set.endofqueue(args[0], args[1])
				else:
					return xmlrpclib.Fault(1, "Proper number of parameters not given")
			elif (cmd == "volume"):
				if len(args) == 1:
					return cmdint.set.volume(args[0])
				else:
					return xmlrpclib.Fault(1, "Proper number of parameters not given")
			else:
				return xmlrpclib.Fault(1, "Invalid command in the set realm")

# vim:ts=8 sw=8 noet

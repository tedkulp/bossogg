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
import types

class Edit:
	"""
	Edit realm.  These commands are used to mainuplate
	data.  They are generally used for playlist
	manipluation.


	edit("createplaylist"): Create a new empty playlist
	with the given name.  If the playlist is created, the
	new playlistid is returned.  If the playlist exists,
	-1 is returned.

	Parameters:
	* name - string

	Returns:
	int


	edit("removeplaylist"): Removes the given playlist
	and all of it's contents.

	Parameters:
	* playlistid - int

	Returns:
	int (always 1)


	edit("add"): Grab songids from the database and put
	them into the given playlist.

	Parameters:
	* playlistid - integer
	* idtypes - string ("songid", "albumid", "artistid", or "playlistid")
	* theid - int
	* replace - boolean -- If true, will clear the playlist before
	  it adds the new songs.

	Returns:
	* int (always 1)


	edit("remove"): Removes the selected index from the given playlist.

	Parameters:
	* playlistid - int
	* indexid - int -- index to remove

	Returns:
	* int (always 1)


	edit("move"): Moves the song from the first postion to the second
	position displacing the songs below the inserted point.

	Parameters:
	* playlistid - int
	* index1 - int
	* index2 - int

	Returns:
	* int (always 1)


	edit("swap"): Moves the song from the first postion to the second
	position and vice versa.

	Parameters:
	* playlistid - int
	* index1 - int
	* index2 - int

	Returns:
	* int (always 1)


	edit("createplaylistfromqueue"): Creates or modifies a playlist
	with what is in the cureent queue.  If a name is given, the playlist
	if first created and then the songs are added to it.  If a playlistid
	is given, the playlist in the db is first cleared and then the songs
	are added.

	Parameters:
	-- It should be either one or the other. More than one variable will
	cause a fault --
	* playlistid - int
	* name - string

	Returns:
	* int - 1 if everything is ok.  -1 if a name is given that has already
	been used.  In that case, no songs will be added.
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

			if (cmd == "createplaylist"):
				if len(args) == 1:
					return cmdint.edit.createplaylist(args[0])
				else:
					return xmlrpclib.Fault(1, "Proper parameters not given")
			elif (cmd == "removeplaylist"):
				if len(args) == 1:
					return cmdint.edit.removeplaylist(args[0])
				else:
					return xmlrpclib.Fault(1, "Proper parameters not given")
			elif (cmd == "add"):
				if len(args) == 4:
					return cmdint.edit.add(args[0], args[1], args[2], args[3])
				else:
					return xmlrpclib.Fault(1, "Proper parameters not given")
			elif (cmd == "remove"):
				if len(args) == 2:
					return cmdint.edit.remove(args[0], args[1])
				else:
					return xmlrpclib.Fault(1, "Proper parameters not given")
			elif (cmd == "move"):
				if len(args) == 3:
					return cmdint.edit.move(args[0], args[1], args[2])
				else:
					return xmlrpclib.Fault(1, "Proper parameters not given")
			elif (cmd == "swap"):
				if len(args) == 3:
					return cmdint.edit.swap(args[0], args[1], args[2])
				else:
					return xmlrpclib.Fault(1, "Proper parameters not given")
			elif (cmd == "createplaylistfromqueue"):
				if len(args) == 1:
					if isinstance(args[0], types.StringType):
						return cmdint.edit.createplaylistfromqueue(name=args[0])
					elif isinstance(args[0], types.IntType):
						return cmdint.edit.createplaylistfromqueue(playlistid=args[0])
					else:
						return xmlrpclib.Fault(1, "Proper parameters not given")
				else:
					return xmlrpclib.Fault(1, "Proper parameters not given")
			else:
				return xmlrpclib.Fault(1, "This is not a valid command")

# vim:ts=8 sw=8 noet

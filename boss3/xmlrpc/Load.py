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
import types
import boss3.xmlrpc.bossxmlrpclib as xmlrpclib

class Load:

	"""
	Load realm.  These functions have to do with loading and saving
	songs in the main queue/playlist.  Until there are user-level
	queues, all users will be inserting songs into the main playlist
	so that they will be picked up and played in the order they
	are queued.


	load("artist"): Loads all the songs for a particular artist into
	the queue in a year,albumname,tracknum,songname order. If
	emptyqueue is given (1 as int or any string), it will clear out
	the queue first before adding the songs.  Will return a fault if
	the incorrect number of parameters are given.

	Parameters:
	* artistid - int
	* emptyqueue - int, string or boolean(optional)

	Returns:
	* int (always 1)


	load("album"): Loads all the songs for a particular album into
	the queue in a tracknum,songname order.  If emptyqueue is given
	(1 as int or any string), it will clear out the queue first before
	adding the songs.  Will return a fault if the incorrect number of
	parameters are given.

	Parameters:
	* albumid - int
	* emptyqueue - int, string or boolean(optional)

	Returns:
	* int (always 1)


	load("song"): Loads into given song into the queue.  If emptyqueue
	is given (1 as int or any string), it will clear out the queue first
	before adding the song.  Will return a fault if the incorrect number
	of parameters are given.

	Parameters:
	* songid - int
	* emptyqueue - int, string or boolean(optional)

	Returns:
	* int (always 1)


	load("playlist"): Loads the songs in the given playlist into the
	queue.  If emptyqueue is given (1 as int or any string), it will
	clear out the queue first before adding the song.  Will return a
	fault if the incorrect number of parameters are given.

	Parameters:
	* playlistid - int
	* emptyqueue - int, string or boolean(optional)

	Returns:
	* int (always 1)

	
	load("move"): Moves a song from one index position to a new index
	position, displacing the song that is currently there and moving
	it down one position.  Will return a fault if either index is out
	of range, or if the incorrect number of parameters are given.

	Parameters:
	* oldindex - int
	* newindex - int

	Returns:
	* int (always 1) TODO: Modify this result to show if it actually
	moved or not.


	load("remove"): Remove a song at the given position.  It will not
	remove the currently playing song.  Will return a fault if index it
	out of range or if the proper parameters are not given.

	Parameters:
	* index - int

	Returns:
	* int (always 1) TODO: Modify this result to show if it actually
	moved or not.


	load("shuffle"): Shuffles the queue.  Though, the shuffle is a little
	weird in that it will only shuffle the songs below the current position.
	This is mostly for internal reasons, but it's also silly to shuffle
	songs that you've already played.

	Parameters:
	* None

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

			if (cmd == "artist"):
				if len(args) > 0:
					arg1 = False
					if len(args) > 1:
						if isinstance(args[1], types.StringType):
							arg1 = True
						elif isinstance(args[1], types.IntType) and args[1] == 1:
							arg1 = True
					return cmdint.load.doQueueAdd("artistid", args[0], arg1)
				else:
					return xmlrpclib.Fault(1, "Invalid number of parameters given")
			elif (cmd == "album"):
				if len(args) > 0:
					arg1 = False
					if len(args) > 1:
						if isinstance(args[1], types.StringType):
							arg1 = True
						elif isinstance(args[1], types.IntType) and args[1] == 1:
							arg1 = True
					return cmdint.load.doQueueAdd("albumid", args[0], arg1)
				else:
					return xmlrpclib.Fault(1, "Invalid number of parameters given")
			elif (cmd == "song"):
				if len(args) > 0:
					arg1 = False
					if len(args) > 1:
						if isinstance(args[1], types.StringType):
							arg1 = True
						elif isinstance(args[1], types.IntType) and args[1] == 1:
							arg1 = True
					return cmdint.load.doQueueAdd("songid", args[0], arg1)
				else:
					return xmlrpclib.Fault(1, "Invalid number of parameters given")
			elif (cmd == "playlist"):
				if len(args) > 0:
					arg1 = False
					if len(args) > 1:
						if isinstance(args[1], types.StringType):
							arg1 = True
						elif isinstance(args[1], types.IntType) and args[1] == 1:
							arg1 = True
					return cmdint.load.doQueueAdd("playlistid", args[0], arg1)
				else:
					return xmlrpclib.Fault(1, "Invalid number of parameters given")
			elif (cmd == "move"):
				if len(args) == 2 and isinstance(args[0], types.IntType) and isinstance(args[1], types.IntType):
					return cmdint.load.move(args[0], args[1])
				else:
					return xmlrpclib.Fault(1, "Invalid number or invalid type of parameters given")
			elif (cmd == "remove"):
				if len(args) == 1 and isinstance(args[0], types.IntType):
					return cmdint.load.remove(args[0])
				else:
					return xmlrpclib.Fault(1, "Invalid number or invalid type of parameters given")
			elif (cmd == "shuffle"):
				return cmdint.load.shuffle()
			else:
				return xmlrpclib.Fault(1, "Invalid command in the load realm")


# vim:ts=8 sw=8 noet

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
from boss3.util import UTFstring
import boss3.xmlrpc.bossxmlrpclib as xmlrpclib
import types

class List:

	"""
	List realm.  Command which deal with querying information
	from the database for the client to display.  These are safe
	query commands that don't really pose a security threat and can
	be given access to even to the lowliest of users.  These
	commands will be locked out by certain database functions.

	
	list("artists"): Displays a list of all the artists in the
	database.  Ordered in alphabetical order.

	Parameters:
	* anchor - string(optional)

	Retruns:
	* Array
	  * Struct
	    * artistname - string(base64)
	    * artistid - int

	
	list("albums"): Returns a list of all the albums in the
	database for the given artistid.  Anchor will only return
	albumnames that start with the given anchor string.

	Parameters:
	* artistid - int
	* anchor - string (optional)

	Retruns:
	* Array
	  * Struct
	    * albumname - string(base64)
	    * albumid - int
	    * albumyear - int
	    * metaartist - int (1 = True, 0 = False)


	list("albumsbygenre"): Returns a list of all the albums in the
	database that have songs with the given genreid.  Anchor will
	only return albumnames that start with the given anchor string.

	Parameters:
	* genreid - int
	* anchor - string (optional)

	Retruns:
	* Array
	  * Struct
	    * albumname - string(base64)
	    * albumid - int
	    * albumyear - int
	    * metaartist - int -- Always 0


	list("songs"): Returns a list of all the albums in the
	database for the given artistid or albumid.  Anchor will
	only return artistname or albumnames that start with
	the given anchor string.  

	Parameters:
	* idtype - string("artistid", "albumid" or "playlistid")
	* id - int
	* anchor - string(optional)

	Returns:
	* Array
	  * Struct
	    * songid - int
	    * artistid - int
	    * albumid - int
	    * songname - string(base64)
	    * filename - string(base64)
	    * songlength - float
	    * filesize - int
	    * tracknum - int
	    * timesplayed - int
	    * bitrate - float
	    * albumname - string(base64)
	    * albumyear - int
	    * artistname - string(base64)


	list("users"): Returns a list of usrs that are logged
	into the server currently.

	Parameters:
	* None

	Returns:
	* Array
	  * Struct
	    * username - string
	    * minconnected - float
	

	list("songinfo"): Gets the information for a song.
	If no songid is given, the songid of current song
	is used.

	Parameters:
	* songid - int(optional)

	Returns:
	* Struct
	  * songid - int
	  * albumid - int
	  * artistid - int
	  * artistname - string(base64)
	  * albumname - string(baes64)
	  * songname - string(base64)
	  * bitrate - int
	  * songlength - int
	  * tracknum - int
	  * filesize - int
	  * timesplayeed - int
	  * filename - stirng(base64)
	  * weight - float
	  * flags - int
	  * albumyear - int
	  * genres - Array
	    * Dictionary
	      * genreid - int
	      * genrename - string
	  
	  -- The following aren't necessarily returned --

	  * timesstarted - int
	  * timesplayed - int
	  * timesrequested - int
	  * percentagecompleted - float
	
	list("queue"): Returns the songs currently in the
	queue.  If showallinfo is 1, then is will return
	all the info from list("songinfo") plus the list("queue")
	info.

	Parameters:
	* showallinfo - int(optional - 1 or 0 - defaults to 0)
	* numbertoreturn - int(optional)

	Returns:
	* Array
	  * Struct
	    * index - int
	    * songid - int
	    (along with this info if showallinfo is 1)
	    * albumid - int
	    * artistid - int
	    * artistname - string(base64)
	    * albumname - string(baes64)
	    * songname - string(base64)
	    * bitrate - float
	    * songlength - float
	    * tracknum - int
	    * filesize - int
	    * timesplayeed - int
	    * filename - stirng(base64)
	    * weight - float
	    * flags - int
	    * albumyear - int
	    * timesstarted - int
	    * timesplayed - int
	    * timesrequested - int
	    * percentagecompleted - float
	    * genres - Array
	      * Dictionary
	        * genreid - int
	        * genrename - string

	list("top"): Gets most played aritst, album, and
	song information.  If no numbertoget is given, it
	defaults to 10 results.

	Parameters:
	* type - string("artists", "albums", "songs")
	* numbertoget - int(optional)

	Returns:
	* Struct
	  * count - int
	  * artistname - string(base64)
	  * albumname - string(baes64 - only when "album" or "song" is requested)
	  * songname - string(base64 - only when "song" is requested)


	list("stats"): Returns some stats about the catelog of songs
	and the songs that have been played.

	Parameters:
	* None

	Returns:
	* Struct
	  * numartists - int
	  * numalbums - int
	  * numsongs - int
	  * sumfilesize - int
	  * sumsec - double
	  * avgfilesize - double
	  * avgsec - double
	  * songsplayed - int
	  * songsstarted - int
	

	list("playlists"): Returns the saved playlists in the database.

	Parameters:
	* None

	Returns:
	* Array
	  * Struct
	    * playlistsid - int
	    * playlistname - string
	    * userid - int
	

	list("genres"): Returns the list of genres in the database.

	Parameters:
	* None

	Returns:
	* Array
	  * Struct
	    * genreid - int
	    * genrename - string
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

			if (cmd == "artists"):
				tmp = None
				if (len(args)>0):
					tmp = cmdint.list.artists(args[0])
				else:
					tmp = cmdint.list.artists()
				for i in tmp:
					if "artistname" in i:
						i['artistname'] = UTFstring.encode(i['artistname'])
				return tmp
			elif (cmd == "albums"):
				tmp = None
				#if (len(args)>0):
				try:
					tmp = cmdint.list.albums(args[0],args[1])
				except Exception:
					try:
						tmp = cmdint.list.albums(args[0])
					except Exception:
						#return xmlrpclib.Fault(2, "Error in given parameters")
						tmp = cmdint.list.albums()
				for i in tmp:
					if "albumname" in i:
						i['albumname'] = UTFstring.encode(i['albumname'])
					if "artistname" in i:
						i['artistname'] = UTFstring.encode(i['artistname'])
				return tmp
				#else:
				#	return xmlrpclib.Fault(1, "No artistid given")
			elif (cmd == "albumsbygenre"):
				tmp = None
				#if (len(args)>0):
				print "%s" % args
				try:
					tmp = cmdint.list.albums(genreid=args[0],anchor=args[1])
				except Exception:
					try:
						tmp = cmdint.list.albums(genreid=args[0])
					except Exception:
						return xmlrpclib.Fault(2, "Error in given parameters")
				for i in tmp:
					if "albumname" in i:
						i['albumname'] = UTFstring.encode(i['albumname'])
				return tmp
			elif (cmd == "songs"):
				tmp = None
				if (len(args) > 1):
					type = args[0]
					someid = args[1]
					if (type == "artistid" or type == "albumid" or type == "playlistid"):
						if (type == "artistid"):
							try:
								tmp = cmdint.list.songs(artistid=someid,anchor=args[2])
							except Exception:
								tmp = cmdint.list.songs(artistid=someid)
						elif (type == "albumid"):
							try:
								tmp = cmdint.list.songs(albumid=someid,anchor=args[2])
							except Exception:
								tmp = cmdint.list.songs(albumid=someid)
						elif (type == "playlistid"):
							try:
								tmp = cmdint.list.songs(playlistid=someid,anchor=args[2])
							except Exception:
								tmp = cmdint.list.songs(playlistid=someid)
						for i in tmp:
							for j in i.keys():
								i[j] = UTFstring.encode(i[j])
						return tmp
					else:
						return xmlrpclib.Fault(2, "%s is not a valid type" % type)
				else:
					return xmlrpclib.Fault(1, "No artistid or albumid given")
			elif (cmd == "users"):
				return cmdint.list.users()
			elif (cmd == "stats"):
				return cmdint.list.stats()
			elif (cmd == "playlists"):
				return cmdint.list.playlists()
			elif (cmd == "genres"):
				return cmdint.list.genres()
			elif (cmd == "songinfo"):
				tmp = None
				if len(args) > 0:
					songid = args[0]
					tmp = cmdint.list.songinfo(songid)
				else:
					tmp = cmdint.list.songinfo()
				for i in tmp.keys():
					tmp[i] = UTFstring.encode(tmp[i])
				return tmp
			elif (cmd == "queue"):
				tmp = None
				if len(args) >= 0 and len(args) < 3:
					if len(args) == 2:
						if isinstance(args[0], types.IntType) and isinstance(args[1], types.IntType):
							if args[0] == 1:
								args[0] = True
							tmp = cmdint.list.queue(args[0], args[1])
						else:
							return xmlrpclib.Fault(1, "Proper types not given")
					elif len(args) == 1:
						if isinstance(args[0], types.IntType):
							if args[0] == 1:
								args[0] = True
							tmp = cmdint.list.queue(args[0])
						else:
							return xmlrpclib.Fault(1, "Proper types not given")
					else:
						tmp = cmdint.list.queue()
					if isinstance(tmp, types.ListType):
						for i in tmp:
							for j in i.keys():
								i[j] = UTFstring.encode(i[j])
					return tmp
				else:
					return xmlrpclib.Fault(1, "Proper number of parameters not given")
			elif (cmd == "top"):
				tmp = None
				if len(args) > 0 and len(args) < 3:
					if len(args) == 2:
						if isinstance(args[0], types.StringType) and isinstance(args[1], types.IntType):
							tmp = cmdint.list.top(args[0], args[1])
						else:
							return xmlrpclib.Fault(1, "Proper types not given")
					else:
						if isinstance(args[0], types.StringType):
							tmp = cmdint.list.top(args[0])
						else:
							return xmlrpclib.Fault(1, "Proper types not given")
					for i in tmp:
						for j in i.keys():
							i[j] = UTFstring.encode(i[j])
					return tmp
				else:
					return xmlrpclib.Fault(1, "Proper number of parameters not given")

# vim:ts=8 sw=8 noet

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

import Player
import version
from boss3.util import bosslog
from boss3.util.Session import *

log = bosslog.getLogger()

class CommandInterface:

	"""Interface for all commands in the server.  All external interfaces
	(xmlrpc, jabber, etc.) must only call commands from this class.  This should
	only be instantiated once per process.  It's best use is to put it in some kind
	of global setting so that it's easily called from all external interfaces.
	"""

	player = None
	dbh = None
	command = None
	list = None
	load = None
	db = None
	util = None
	info = None
	auth = None
	set = None
	edit = None

	def __init__(self, player, dbh):
		self.player = player
		session = Session()
		session['cmdint'] = self
		self.dbh = dbh
		self.control = Control(player, dbh)
		self.list = List(player, dbh)
		self.load = Load(player, dbh)
		self.db = DB(player, dbh)
		self.util = Util(player, dbh)
		self.info = Info(player, dbh)
		self.auth = Auth(player, dbh)
		self.set = Set(player, dbh)
		self.edit = Edit(player, dbh)

class Control:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh

	def play(self):
		"""Sends the "play" command to the current player thread."""
		self.player.push_cmd("play")

	def playIndex(self,index):
		"""
		Sends the "playindex" command to the current player thread.
		This tells the player to immediatly start playing the given
		index.

		Parameters:
		* index - int
		"""
		self.player.push_cmd("playindex"+str(index))

	def pause(self):
		"""Sends the "pause" command to the current player thread."""
		self.player.push_cmd("pause")

	def stop(self):
		"""Sends the "stop" command to the current player thread."""
		self.player.push_cmd("stop")

	def next(self):
		"""Sends the "next" command to the current player thread."""
		self.player.push_cmd("next")

class List:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh

	def artists(self, anchor=""):
		"""
		Returns a list of all the artists in the
		database.  Anchor will only return
		artistnames that start with the given
		anchor string.

		Parameters:
		* anchor - string(optional)

		Returns:
		* Array
		  * Dictionary
		    * artistid - int
		    * artistname - string
		"""
		#print self.dbh.listArtists(anchor)
		return self.dbh.listArtists(anchor)

	def albums(self, artistid, anchor=""):
		"""
		Returns a list of all the albums in the
		database for the given artistid.  Anchor
		will only return albumnames that start
		with the given anchor string.

		Parameters:
		* artistid - int
		* anchor - string(optional)

		Returns:
		* Array
		  * Dictionary
		    * albumid - int
		    * albumname - string
		    * albumyear - int
		    * metaartist - int (1 = true, 0 = false)
		"""
		return self.dbh.listAlbums(artistid, anchor)

	def songs(self, artistid=None, albumid=None, playlistid=None, anchor="", getgenres=True):
		"""
		Returns a list of all the albums in the
		database for the given artistid or albumid.
		Anchor will only return artistname or albumnames
		that start with the given anchor string.  

		Parameters:
		* artistid - int(optional)
		* albumid - int(optional)
		* playlistid - int(optional)
		* anchor - string(optional)
		* getgenres - bool(optional)

		Returns:
		* Array
		  * Dictionary
		    * songid - int
		    * artistid - int
		    * albumid - int
		    * songname - string
		    * filename - string
		    * songlength - int
		    * filesize - int
		    * tracknum - int
		    * timesplayed - int
		    * bitrate - float
		    * albumname - string
		    * albumyear - int
		    * artistname - string
		    * genres - Array
		      * Dictionary
		        * genreid - int
			* genrename - string
		"""
		return self.dbh.listSongs(artistid, albumid, playlistid, anchor, getgenres)

	def users(self):
		"""
		Returns a list of all the current users logged in
		with a valid session.

		Parameters:
		* none

		Returns:
		* Array
		  * Dictionary
		    * username - string
		    * minconnected - float
		"""
		session = Session()
		return session['usrmgr'].listUsers()

	def songinfo(self, songid=None):
		"""
		Sends the get SongInfo command to the database object.
		If no songid is present, songid of current song from the 
		player thread is used.

		Parameters:
		* songid - int(optional)

		Returns:
		* Dictionary
		  * songid - int
		  * albumid - int
		  * artistid - int
		  * artistname - string
		  * albumname - string
		  * songname - string
		  * bitrate - int
		  * songlength - int
		  * tracknum - int
		  * filesize - int
		  * timesplayed - int
		  * filename - string
		  * weight - float
		  * flags - int
		  * albumyear - int
		  * genres - Array
		    * Dictionary
		      * genreid - int
		      * genrename - string

		  -- The following aren't necessarily returned --

		  * timestarted - int
		  * timesplayed - int
		  * timesrequested - int
		  * percentagecompleted - float
		"""
		if songid is None:
			return self.dbh.getSongInfo(self.player.songid)
		else:
			return self.dbh.getSongInfo(songid)
	
	def queue(self, showall=False, numtoreturn=-1):
		"""
		Returns the songs currently in the
		queue.  If showallinfo is 1, then is will return
		all the info from list("songinfo") plus the list("queue")
		info.

		Parameters:
		* showallinfo - True/False(defaults to False)
		* numbertoreturn - int(defaults to -1)

		Returns:
		* List
		  * Dictionary
		    * index - int
		    * songid - int
		    (along with this info if showallinfo is 1)
		    * albumid - int
		    * artistid - int
		    * artistname - string
		    * albumname - string
		    * songname - string
		    * bitrate - float
		    * songlength - float
		    * tracknum - int
		    * filesize - int
		    * timesplayeed - int
		    * filename - stirng
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
		"""
		result = []
		count = 0
		theids = self.player.songqueue.getSongIDs()
		for i in theids:
			if numtoreturn > -1 and count >= numtoreturn:
				break
			if showall is True:
				info = self.songinfo(i)
			else:
				info = {}
				info['songid'] = i
			info['index'] = count
			result.append(info)
			count += 1
		return result
	
	def top(self, type="artists", numbertoget=10):
		"""
		Returns top artists, albums and songs information from the
		history table.

		Parameters:
		* type - string ("artists", "albums", or "songs")
		* numbertoget - int (optional - defaults to 10)

		Returns:
		* Dictionary
		  * count - int
		  * artistname - string
		  * albumname - string (only when "album" or "song" is requested
		  * songname - string (only when "song" is requested)
		"""
		if type == "artists":
			return self.dbh.topArtists(numbertoget)
		elif type == "albums":
			return self.dbh.topAlbums(numbertoget)
		elif type == "songs":
			return self.dbh.topSongs(numbertoget)
		else:
			return {}
	
	def stats(self):
		"""
		Returns some stats about the catelog of songs and the songs
		that have been played.

		Parameters:
		* none

		Returns:
		* Dictionary
		  * numartists - int
		  * numalbums - int
		  * numsongs - int
		  * sumfilesize - int
		  * sumsec - double
		  * avgfilesize - double
		  * avgsec - double
		  * songsplayed - int
		  * songsstarted - int
		"""
		return self.dbh.getStats()
	
	def playlists(self):
		"""
		Returns the saved playlists.

		Parameters:
		* none

		Returns:
		* Array
		  * Dictionary
		    * plyalistid - int
		    * playlistname - string
		    * userid - int
		"""
		return self.dbh.listPlaylists()

	def genres(self):
		"""
		Returns the list of genres.

		Parameters:
		* none

		Returns:
		* Array
		  * Dictionary
		    * genreid - int
		    * genrename - string
		"""
		return self.dbh.listGenres()

class Info:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh

	def status(self):
           	"""
		Gets the status of the current player thread.

		Parameters:
		* none

		Returns:
		* Dictionary
		TODO:Detail the returned status elements here
		"""
		return self.player.getStatus()
	
	def endofqueue(self):
		"""
		Gets the currently selected endofqueue action.

		Parameters:
		* none

		Returns:
		* String
		"""
		session = Session()
		if session.hasKey('cfg'):
			return session['cfg'].get("endofqueue")
		else:
			return ""

class Auth:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh

	def login(self, username=None, password=None):
		"""
		Send the authUser command to the current user manager
		Requires a username and password as arguments and returns
		a session id string to be used by stateless clients

		Parameters:
		* username - string
		* password - string

		Returns:
		* string
		"""
		session = Session()
		return session['usrmgr'].authUser(username, password)
	
	def logout(self, sessionid):
		"""
		Logs the user out and clears the session.

		Parameters:
		* sessionid - string

		Returns:
		* int (1 if true, 0 if true)
		"""
		session = Session()
		return session['usrmgr'].cancelSession(sessionid)
	
	def touch(self, sessionid):
		"""
		Touches the session so that it doesn't get
		automatically cleared.

		Parameters:
		* sessionid - string

		Returns:
		* int (1 if true, 0 if true)
		"""
		session = Session()
		return session['usrmsg'].touchSession(sessionid)

class DB:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh

	def importcache(self):
		"""
		Send the importCache command to the database manager,
		which will show a list of all songs in the database.

		Parameters:
		* none

		Returns:
		* Array
		  * Dictionary
		    * filename - string
		    * modifieddate - string
		"""
		return self.dbh.importCache()

	def importstart(self):
		self.dbh.importStart()

	def importend(self):
		self.dbh.importEnd()
	
	def importcancel(self):
		self.dbh.importCancel()
	
	def importsongs(self, arrayofsongs):
		return self.dbh.importSongs(arrayofsongs)

	def importdelete(self, arrayofsongs):
		return self.dbh.importDelete(arrayofsongs)

	def importupload(self, filename, songdata=None):
		return self.dbh.importUpload(filename, songdata)

	def getCDDB(self, device):
		return self.dbh.getCDDB(device)
		
	def pyrip(self, tags, filenames, device):
		return self.dbh.pyrip(tags, filenames, device)

	def pyrip_update(self):
		return self.dbh.pyrip_update()
	
class Load:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh

	def artist(self, theid, replace=False):
		"""Convienence function for calling doQueueAdd()"""
		return self.doQueueAdd("artistid", theid, replace)

	def album(self, theid, replace=False):
		"""Convienence function for calling doQueueAdd()"""
		return self.doQueueAdd("albumid", theid, replace)

	def song(self, theid, replace=False):
		"""Convienence function for calling doQueueAdd()"""
		return self.doQueueAdd("songid", theid, replace)

	def playlist(self, theid, replace=False):
		"""Convienence function for calling doQueueAdd()"""
		return self.doQueueAdd("playlistid", theid, replace)

	def doQueueAdd(self, idtype, theid, replace=False):
		"""
		Grab songids from the database and put them into the
		current player thread's queue.
		
		Parameters:
		* idtypes can be: "songid", "albumid", "artistid", or "playlistid".
		* theid is an integer id value.
		* replace is a boolean that, if true, will clear the queue before
		  it adds the new songs.

		Returns:
		* int (always 1)
		"""
		ids = self.dbh.getIds(idtype, theid)
		#log.debug("queue", "clear queue?: %s", replace)
		#log.debug("queue", ""ids: %s", ids)
		if replace is True:
			self.player.queueClear()
		for i in ids:
			self.dbh.setQueueHistoryOnId(i['songid'])
			self.player.queueSong(i)
		return 1

	def move(self, oldindex, newindex):
		"""
		Moves song at oldindex position to newindex position, shiftings
		all of the songs down one position.

		Parameters:
		* oldindex - int
		* newindex - int

		Returns:
		* int (always 1)
		"""
		self.player.queueMove(oldindex, newindex)
		return 1
	
	def remove(self, index):
		"""
		Removes the song at the given position.

		Parameters:
		* index - int

		Returns:
		* int (always 1)
		"""
		self.player.queueRemove(index)
		return 1
	
	def shuffle(self):
		"""
		Shuffles the songs in the queue.  However, it
		will only shuffle songs after the current position,
		so as not to cause repeats. TODO: Make it shuffle
		all songs if the current position is the last song
		in the queue.

		Parameters:
		* none

		Returns:
		* int (always 1)
		"""
		self.player.queueShuffle()
		return 1

class Set:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh
	
	def endofqueue(self,endofqueueaction="allstraight", endofqueueparam=""):
		"""
		Sets the endofqueue action.

		Parameters:
		* endofqueueaction - string
		* endofqueueparam - string
		
		Returns:
		* int (always 1)
		"""
		session = Session()
		if session.hasKey('cfg'):
			session['cfg'].set("endofqueue",endofqueueaction)
			session['cfg'].set("endofqueueparam",endofqueueparam)
		return 1
		
	def saveconfig(self):
		"""
		Save the bossogg.conf file back to the filesystem overwriting
		the old one.

		Parameters:
		* none
		
		Returns:
		* int (always 1)
		"""
		session = Session()
		if session.hasKey('cfg'):
			session['cfg'].save()
		return 1
	
	def shutdown(self):
		"""
		Sends the signal the shut the server down.

		Parameters:
		* none

		Returns:
		* int (always 1)
		"""
		log.debug("misc", "Shutdown Request Received")
		session = Session()
		session['shutdown'] = 1
		return 1
	
	def volume(self,volume):
		"""
		Sets the playback volume.

		Parameters:
		* volume - int

		Returns:
		* int (always 1)
		"""
		self.player.setVolume(volume)
		return 1

class Util:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh

	def version(self):
		"""
		Returns version information about the server.

		Parameters: None

		Returns:
		* Dictionary
		-- "version" - string
		-- "name" - string
		"""

		tmp = {}
		tmp['version'] = version.getVersion()
		tmp['name'] = version.getName()
		return tmp

class Edit:

	player = None
	dbh = None

	def __init__(self, player, dbh):
		self.player = player
		self.dbh = dbh
	
	def createplaylist(self, name):
		"""
		Creates an empty playlist with the given name.

		Parameters:
		name - string

		Returns:
		int - The playlistsid of the created playlist.  Will
		      return -1 is the playlist already exists.
		"""

		return self.dbh.createPlaylist(name)
	
	def removeplaylist(self, playlistid):
		"""
		Removes the playlist and all of its contents.

		Parameters:
		playlistid - int

		Returns:
		int - (always 1)
		"""
		self.dbh.removePlaylist(playlistid)
		return 1
	
	def createplaylistfromqueue(self, playlistid=None, name=None):
		"""
		Create a playlist from the songs in the existing queue.
		If a playlistid is given, the given playlist is cleared
		and then all the songs are added.  If name is given,
		the playlist is created and the songs are added.

		Parmaeters:
		* playlistid - int(optional)
		* name - string(optional)

		Returns:
		int - 1 is playlist is created -- -1 if the playlist
		name given exists.
		"""
		if playlistid is not None:
			self.dbh.playlistClear(playlistid)
			ids = self.player.songqueue.getSongIDs()
			for i in ids:
				self.add(playlistid, "songid", i)
		elif name is not None:
			playlistid = self.createplaylist(name)
			if playlistid > -1:
				ids = self.player.songqueue.getSongIDs()
				for i in ids:
					self.add(playlistid, "songid", i)
			else:
				return -1
		return 1
	
	def add(self, playlistid, idtype, theid, replace=False):
		"""
		Grab songids from the database and put them into the
		given playlist.
		
		Parameters:
		* playlistid is an integer
		* idtypes can be: "songid", "albumid", "artistid", or "playlistid".
		* theid is an integer id value.
		* replace is a boolean that, if true, will clear the playlist before
		  it adds the new songs.

		Returns:
		* int (always 1)
		"""
		ids = self.dbh.getIds(idtype, theid)
		#log.debug("queue", "clear queue?: %s", replace)
		#log.debug("queue", "ids: %s", ids)
		if replace is True:
			self.dbh.playlistClear(theid)
		for i in ids:
			self.dbh.addSongToPlaylist(playlistid, i['songid'])
		return 1
	
	def remove(self, playlistid, indexid):
		"""
		Removes the selected index from the given playlist.

		Parameters:
		* playlistid - int
		* indexid - int of the index to remove

		Returns:
		* int (always 1)
		"""
		self.dbh.removeSongFromPlaylist(playlistid, indexid)
		return 1
	
	def move(self, playlistid, index1, index2):
		"""
		Moves the song from the first postion to the second
		position displacing the songs below the inserted point.

		Parameters:
		* playlistid - int
		* index1 - int
		* index2 - int

		Returns:
		* int (always 1)
		"""
		self.dbh.moveSongInPlaylist(playlistid, index1, index2, False)
		return 1

	def swap(self, playlistid, index1, index2):
		"""
		Moves the song from the first postion to the second
		position and vice versa.

		Parameters:
		* playlistid - int
		* index1 - int
		* index2 - int

		Returns:
		* int (always 1)
		"""
		self.dbh.moveSongInPlaylist(playlistid, index1, index2, True)
		return 1

# vim:ts=8 sw=8 noet

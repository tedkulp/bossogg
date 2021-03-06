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

import sqlite
import time
import os, sys
from boss3.bossexceptions import EndOfQueueException
from boss3.util import bosslog
from boss3.util.Session import *
import time
from boss3.util import UTFstring
import os
import string
import types
import thread
import threading
from boss3.metadata import id3
from boss3.metadata.id3 import getTag
try:
	import boss3.ripper.ripper
	from boss3.ripper.ripper import getCDDB
	from boss3.ripper.ripper import pyrip
	from boss3.ripper.ripper import pyrip_update
except Exception:
	pass

log = bosslog.getLogger()

sql_lock = threading.RLock()


class Database:
	conn = ""
	dbname = ""
	songcache = []
	curindex = -1
	genrecache = {}
	artistcache = {}
	albumcache = {}
	getgenrestatus = False
	getartiststatus = False
	getmetaartiststatus = False
	getalbumstatus = False
	getsongstatus = False
	#cursong = None
	import_cursor = None
	tables = {}

	class _Cursor(sqlite.Cursor):
		nolock=0
		
		def getcaller(self):
			f=sys._getframe(1)
			f=f.f_back
			return os.path.basename(f.f_code.co_filename), f.f_lineno

		def execute(self, SQL, *args, **kwargs):
			needlock=0

			#if len(SQL)>0 and SQL.split()[0].lower() in ["delete", "update", "insert", "commit"] and not self.nolock:
			needlock=1

			if needlock:
				sql_lock.acquire()
				log.debug("lock", "Acquire lock for database writes", stack=1)
			try:
				log.debug("sqlquery", "SQL: "+SQL, stack=1, *args)
				sqlite.Cursor.execute(self, SQL, *args)
			except:
				log.exception("SQL ERROR")
				if "raise_except" in kwargs and kwargs["raise_except"] == 1:
					if needlock:
						sql_lock.release()
						log.debug("lock", "Release lock for database writes", stack=1)
					raise

			if needlock:
				sql_lock.release()
				log.debug("lock", "Release lock for database writes", stack=1)

		def begin(self):
			self.execute("BEGIN TRANSACTION")

		def commit(self):
			self.execute("COMMIT TRANSACTION")
			
		def rollback(self):
			self.execute("ROLLBACK TRANSACTION")


	def _cursor(self):
		self.conn._checkNotClosed("cursor")
		return self._Cursor(self.conn, self.conn.rowclass)

	def loadTableStructures(self):
		self.tables = {}
		log.debug("funcs", "Database.tableStructures")
		cursor = self.conn.cursor()
		cursor.execute("select name,sql from sqlite_master where type='table'")
		for row in cursor.fetchall():
			self.tables[row[0]] = []
			sql = row[1].split("\n")
			for line in sql[1:-1]:
				data = line.split()
				field = data[0]
				self.tables[row[0]].append(field)
		cursor.close()
		log.debug("import", "Got Table data %s", self.tables)

	def connect(self, autocommit=True):
		if ( (self.conn == None or self.conn == "") and self.dbname != ""):
			self.conn = sqlite.connect(db=self.dbname, mode=755, autocommit=autocommit)
			self.conn.cursor=self._cursor


	def disconnect(self):
		if (self.conn != None or self.conn != ""):
			self.conn.close()
			self.conn = None

	def runScript(self,SQL):
		cursor = self.conn.cursor()
		cursor.execute(SQL)
		#cursor.commit()
		cursor.close()

	def getSchemaVersion(self):
		result = -1
		cursor = self.conn.cursor()
		SQL = "select versionnumber from version"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result = row['versionnumber']
		return result

	def setSchemaVersion(self, versionnumber):
		cursor = self.conn.cursor()
		SQL = "update version set versionnumber = %s"
		cursor.execute(SQL, versionnumber)

	def loadSongCache(self):
		log.debug("funcs", "Database.loadSongCache() called")
		cursor = self.conn.cursor()
		SQL = """
		SELECT songs.songid, songs.filename, songs.songlength, songs.flags
		FROM songs, albums, artists
		WHERE songs.albumid = albums.albumid and albums.artistid = artists.artistid
		ORDER BY artists.artistname, albums.year, albums.albumname, songs.tracknum, songs.songname"""
		cursor.execute(SQL)
		i = 0
		self.songcache = []
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			self.songcache.append({"filename":row['songs.filename'], "songid":row['songs.songid'], "songlength":row['songs.songlength'], "flags":row['songs.flags']})
			i += 1
		log.debug("cache", "Loaded %s songs into cache", i)

	def getSongCacheSize(self):
		if self.songcache is not None:
			return len(self.songcache)

	def loadState(self,player):
		cursor = self.conn.cursor()
		SQL = "select * from currentstate"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			player.songqueue.setCurrentIndex(row['queueindex'])
		SQL = "select q.songid,s.filename,s.songlength,s.flags from queue q inner join songs s on q.songid = s.songid order by q.indexid"
		cursor.execute(SQL)
		i=0
		for row in cursor.fetchall():
			player.queueSong({"filename":row['s.filename'], "songid":row['q.songid'], "songlength":row['s.songlength'], "flags":row['s.flags']})
			i += 1
		if i == 0:
			player.songqueue.setCurrentIndex(-1)
		#if player.songqueue.currentindex > -1:
		#	player.songqueue.currentindex -= 1

	def saveState(self,player):
		cursor = self.conn.cursor()
		SQL = "delete from currentstate"
		cursor.execute(SQL)
		SQL = "insert into currentstate (queueindex, playlistid, songid, shuffle) values (%s, %s, %s, %s)" 
		cursor.execute(SQL, player.songqueue.getCurrentIndex(),-1,player.songid,0)
		SQL = "delete from queue"
		cursor.execute(SQL)
		queuesongids = player.songqueue.getSongIDs()
		for i in queuesongids:
			SQL = "insert into queue (songid) values (%s)"
			cursor.execute(SQL, i)

	def getNextSong(self, newindex = -1, shuffle = 0):
		try:
			if (newindex == None or newindex < 0):
				self.curindex += 1
				if (shuffle == None or shuffle == 0):
					return self.songcache[self.curindex]
		except Exception:
			raise EndOfQueueException.EndOfQueueException("No songs left... need to go into stop mode")

	def getRandomSong(self):
		cursor = self.conn.cursor()
		SQL = "select songs.songid, songs.filename, songs.songlength, songs.flags from songs order by random() limit 1"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			return {"filename":row['songs.filename'], "songid":row['songs.songid'], "songlength":row['songs.songlength'], "flags":row['songs.flags']}
		raise EndOfQueueException.EndOfQueueException("No songs left... need to go into stop mode")
	
	def getArtistInfo(self, artistid):
		result = {}
		cursor = self.conn.cursor()
		SQL = "select artistid, aristname from artists where artistid = %s"
		cursor.execute(SQL, artistid)
		for row in cursor.fetchall():
			log.debug("sqlresult", "XRow: %s", row)
			result['artistid'] = row['artistid']
			result['artistname'] = row['artistname']
		return result

	def getAlbumInfo(self, albumid):
		result = {}
		cursor = self.conn.cursor()
		SQL = "select a.artistid, a.aristname, al.albumid, al.albumname, al.year from albums al inner join artists a on a.artistid = al.artistid where a.artistid = %s"
		cursor.execute(SQL, albumid)
		for row in cursor.fetchall():
			log.debug("sqlresult", "XRow: %s", row)
			result['artistid'] = row['a.artistid']
			result['artistname'] = row['a.artistname']
			result['albumid'] = row['a.albumid']
			result['albumname'] = row['a.albumname']
			result['albumyear'] = row['al.year']
		return result

	def getSongInfo(self, songids):
		resultarray = []
		cursor = self.conn.cursor()
		whereclause = ""
		for songid in songids:
			whereclause += "songid = %s or " % songid
		whereclause = whereclause[:-4]
		SQL = "select s.songid, al.artistid, s.albumid, s.songname, s.bitrate, s.songlength, s.tracknum, s.filesize, s.timesplayed, s.filename, s.weight, s.flags, al.albumname, al.year, a.artistname, s.metaartistid, m.artistname from songs s inner join albums al on s.albumid = al.albumid inner join artists a on s.artistid = a.artistid outer left join artists m on m.artistid = s.metaartistid where %s" % whereclause
		cursor.execute(SQL)
		for row in cursor.fetchall():
			result = {}
			log.debug("sqlresult", "XRow: %s", row)
			result['songid'] = row['s.songid']
			result['albumid'] = row['s.albumid']
			result['artistid'] = row['al.artistid']
			result['artistname'] = row['a.artistname']
			result['albumname'] = row['al.albumname']
			result['songname'] = row['s.songname']
			result['bitrate'] = row['s.bitrate']
			result['songlength'] = row['s.songlength']
			result['tracknum'] = row['s.tracknum']
			result['filesize'] = row['s.filesize']
			result['timesplayed'] = row['s.timesplayed']
			result['filename'] = row['s.filename']
			result['weight'] = row['s.weight']
			result['flags'] = row['s.flags']
			result['albumyear'] = row['al.year']
			if row['m.artistname'] != None and row['s.metaartistid'] != '-1':
				result['metaartistid'] = row['s.metaartistid']
				result['metaartistname'] = row['m.artistname']
			resultarray.append(result)

		for result in resultarray:
			songid = result['songid']

			SQL = "select count(*) as thecount, type, songid from history where songid = %s group by songid, type order by songid"
			result['timesstarted'] = result['timesplayed'] = result['timesrequested'] = 0
			cursor.execute(SQL, songid)
			for row in cursor.fetchall():
				log.debug("sqlresult", "Row: %s", row)
				if row['type'] == "s":
					result['timesstarted'] = int(row['thecount'])
				elif row['type'] == "p":
					result['timesplayed'] = int(row['thecount'])
				elif row['type'] == "q":
					result['timesrequested'] = int(row['thecount'])

			if result['timesplayed'] and result['timesstarted']:
				result['percentagecompleted'] = (float(result['timesplayed']) / float(result['timesstarted'])) * float(100)
			else:
				result['percentagecompleted'] = float(0)

			result['genres'] = self.fillSongGenreHash(songid)

		#Now sort them in the original order...
		oldresultarray = resultarray
		resultarray = []
		count = 0;
		for songid in songids:
			for i in oldresultarray:
				if i['songid'] == songid:
					i['index'] = count
					resultarray.append(i)
					count += 1
					break
		return resultarray

	def authUser(self, username = "", password = ""):
		result = None
		cursor = self.conn.cursor()
		SQL = "SELECT userid, authlevel FROM users "
		SQL += "WHERE username = %s AND password = %s"
		cursor.execute(SQL, username, password)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result = {"userid":row['userid'],"authlevel":['authlevel']}
		return result

	def listArtists(self, anchor=""):
		result = []
		cursor = self.conn.cursor()
		SQL = "SELECT artistid, artistname FROM artists "
		if (anchor != None and anchor != ""):
			SQL += "WHERE artistname like '%s%%' " % anchor
		SQL += "ORDER BY lower(artistname) ASC"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			if row['artistname'] != '':
				result.append({"artistid":row['artistid'],"artistname":row['artistname']})
		return result

	def listAlbums(self, artistid=None, genreid=None, anchor=""):
		result = []
		cursor = self.conn.cursor()
		#Look for real albums first and stick them at the top of the list

		if artistid != None:
			SQL = "SELECT albumid, albumname, year FROM albums WHERE artistid = %i " % artistid
			if (anchor != None and anchor != ""):
				SQL += "AND albumname like '%s%%%%' " % anchor.replace("'", "\\'")
			SQL += "ORDER BY year, lower(albumname) ASC"
			cursor.execute(SQL)
			for row in cursor.fetchall():
				log.debug("sqlresult", "Row: %s", row)
				result.append({"albumid":row['albumid'],"albumname":row['albumname'],"albumyear":row['year'],"metaartist":0})
			#Now look for metaartist related albums
			SQL = "SELECT a.albumid, a.albumname, a.year FROM albums a INNER JOIN songs s ON a.albumid = s.albumid WHERE s.metaartistid = %s " % artistid

			if (anchor != None and anchor != ""):
				SQL += "AND a.albumname like '%s%%%%' " % anchor.replace("'", "\\'")
			SQL += "ORDER BY a.year, lower(a.albumname) ASC"
			cursor.execute(SQL)
			for row in cursor.fetchall():
				log.debug("sqlresult", "Row: %s", row)
				result.append({"albumid":row['a.albumid'],"albumname":row['a.albumname'],"albumyear":row['a.year'],"metaartist":1})
		elif genreid != None:
			SQL = "SELECT DISTINCT ar.artistid, ar.artistname, a.albumid, a.albumname, a.year FROM artists ar INNER JOIN albums a ON ar.artistid = a.artistid INNER JOIN songs s ON a.albumid = s.albumid INNER JOIN genre_data gd ON gd.songid = s.songid WHERE gd.genreid = %s " % genreid
			if (anchor != None and anchor != ""):
				SQL += "AND albumname like '%s%%%%' " % anchor.replace("'", "\\'")
			SQL += "ORDER BY lower(a.albumname), lower(ar.artistname) ASC"
			cursor.execute(SQL)
			for row in cursor.fetchall():
				log.debug("sqlresult", "Row: %s", row)
				result.append({"artistid":row['ar.artistid'],"artistname":row['ar.artistname'],"albumid":row['a.albumid'],"albumname":row['a.albumname'],"albumyear":row['a.year'],"metaartist":0})
		else:
			#SQL = "SELECT ar.artistid, ar.artistname, a.albumid, a.albumname, a.year FROM albums a INNER JOIN artists ar ON ar.artistid = a.artistid ORDER BY ar.artistname, a.year, lower(a.albumname) ASC"
			SQL = "SELECT ar.artistid, ar.artistname, a.albumid, a.albumname, a.year FROM albums a INNER JOIN artists ar ON ar.artistid = a.artistid ORDER BY lower(a.albumname), ar.artistname ASC"
			cursor.execute(SQL)
			for row in cursor.fetchall():
				log.debug("sqlresult", "Row: %s", row)
				result.append({"artistid":row['ar.artistid'],"artistname":row['ar.artistname'],"albumid":row['a.albumid'],"albumname":row['a.albumname'],"albumyear":row['a.year'],"metaartist":0})
		return result

	def listPlaylists(self):
		result = []
		cursor = self.conn.cursor()
		SQL = "SELECT playlistid, name, userid FROM playlists order by playlistid"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result.append({"playlistid":row['playlistid'],"playlistname":row['name'],"userid":row['userid']})
		return result

	def listGenres(self):
		result = []
		cursor = self.conn.cursor()
		SQL = "SELECT genreid, genrename FROM genres order by genreid"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result.append({"genreid":row['genreid'],"genrename":row['genrename']})
		return result

	def incrementTimesStarted(self, songid):
		cursor = self.conn.cursor()
		SQL = "INSERT INTO HISTORY (songid, type, time) VALUES (%s, 's', %s)"
		cursor.execute(SQL, songid, time.time())

	def incrementTimesPlayed(self, songid):
		cursor = self.conn.cursor()
		SQL = "UPDATE songs SET timesplayed = timesplayed + 1 where songid = %s"
		cursor.execute(SQL, songid)
		SQL = "INSERT INTO HISTORY (songid, type, time) VALUES (%s, 'p', %s)"
		cursor.execute(SQL, songid, time.time())

	def getIds(self, idtype, theid):
		result = []
		cursor = self.conn.cursor()
		SQL = "select s.filename, s.songid, s.songlength, s.flags from songs s "
		if idtype == "artistid":
			SQL += ", albums a where s.artistid = %d and s.albumid = a.albumid order by a.year, a.albumname, s.tracknum, s.songname" % theid
		elif idtype == "albumid":
			SQL += "where albumid = %d order by tracknum, songname" % theid
		elif idtype == "songid":
			SQL += "where songid = %d" % theid
		elif idtype == "playlistid":
			SQL += ", playlistdata p where p.songid = s.songid and p.playlistid = %d order by p.indexid" % theid
		elif idtype == "genreid":
			SQL += "INNER JOIN genre_data gd ON s.songid = gd.songid WHERE gd.genreid = %d ORDER BY s.songid" % theid
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result.append({"filename":row['s.filename'], "songid":row['s.songid'], "songlength":row['s.songlength'], "flags":row['s.flags']})
		#Now grab metaartist related songs if artistid is given
		if idtype == "artistid":
			SQL = "select s.filename, s.songid, s.songlength, s.flags from songs s, albums a where s.metaartistid = %d and s.albumid = a.albumid order by a.year, a.albumname, s.tracknum, s.songname"
			cursor.execute(SQL, theid)
			for row in cursor.fetchall():
				log.debug("sqlresult", "Row: %s", row)
				result.append({"filename":row['s.filename'], "songid":row['s.songid'], "songlength":row['s.songlength'], "flags":row['s.flags']})
		return result

	def setQueueHistoryOnId(self, songid, userid=-1):
		cursor = self.conn.cursor()
		SQL = "INSERT INTO HISTORY (songid, type, time, userid) VALUES (%s, 'q', %s, %s)"
		cursor.execute(SQL, songid, time.time(), userid)

	def createPlaylist(self, name):
		cursor = self.conn.cursor()
		SQL = "SELECT playlistid from playlists where name = %s"
		cursor.execute(SQL, name)
		exists = -1
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			exists = 1
		if (exists == -1):
			now=time.time()
			SQL = "INSERT into playlists (name,userid,create_date,modified_date) values (%s,-1,%s,%s)"
			cursor.execute(SQL, name, now, now)
			SQL = "SELECT playlistid from playlists where name = %s"
			cursor.execute(SQL, name)
			for row in cursor.fetchall():
				exists = row['playlistid']
		else:
			exists = -1
		return exists

	def removePlaylist(self, playlistid):
		cursor = self.conn.cursor()
		SQL = "delete from playlistdata where playlistid = %s"
		cursor.execute(SQL, playlistid)
		SQL = "delete from playlists where playlistid = %s"
		cursor.execute(SQL, playlistid)

	def listSongs(self, artistid=None, albumid=None, playlistid=None, anchor="", getgenres=True):
		log.debug("funcs", "Database.listSongs()")
		result = []
		cursor = self.conn.cursor()
		SQL = "SELECT s.songid, s.artistid, ar.artistname, s.albumid, s.tracknum, s.songname, s.filename, s.filesize, s.bitrate, s.songlength, s.timesplayed, a.albumname, a.year, s.metaartistid, m.artistname"
		if (playlistid != None and playlistid != ""):
			SQL += ", p.indexid"
		SQL += " FROM songs s, albums a, artists ar"
		if (playlistid != None and playlistid != ""):
			SQL += ", playlistdata p"
		SQL += " LEFT OUTER JOIN artists m on s.metaartistid = m.artistid WHERE a.albumid = s.albumid and ar.artistid = s.artistid "
		if (albumid != None and albumid != ""):
			SQL += "AND s.albumid = %i " % albumid
		if (artistid != None and artistid != ""):
			SQL += "AND s.artistid = %i " % artistid
		if (playlistid != None and playlistid != ""):
			SQL += "AND p.playlistid = %i and p.songid = s.songid " % playlistid
		if (anchor != None and anchor != ""):
			SQL += "AND s.songname like '%s%%' " % anchor
		SQL += "ORDER BY"
		if playlistid != None and playlistid != "":
			SQL += " p.indexid,"	
		SQL +=" a.year, lower(a.albumname), s.tracknum, s.songname"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			self.fillSongHash(row, result, getgenres)
		if artistid != None and artistid != "":
			SQL = "SELECT s.songid, s.artistid, ar.artistname, s.albumid, s.tracknum, s.songname, s.filename, s.filesize, s.bitrate, s.songlength, s.timesplayed, a.albumname, a.year, s.metaartistid, m.artistname FROM songs s, albums a, artists ar LEFT OUTER JOIN artists m on s.metaartistid = m.artistid WHERE a.albumid = s.albumid and ar.artistid = s.artistid AND s.metaartistid = %s"
			cursor.execute(SQL, artistid)
			for row in cursor.fetchall():
				self.fillSongHash(row, result)
		return result

	def fillSongHash(self, row, result, getgenres=True):
		log.debug("sqlresult", "Row: %s", row)
		timesplayed = 0
		if row['s.timesplayed'] != None:
			timesplayed = row['s.timesplayed']
		somesong = {"songid":row['s.songid'],"artistid":row['s.artistid'],"albumid":row['s.albumid'],"songname":row['s.songname'],"filename":row['s.filename'],"filesize":row['s.filesize'],"songlength":row['s.songlength'],"tracknum":row['s.tracknum'],"timesplayed":timesplayed,"bitrate":row['s.bitrate'],"albumname":row['a.albumname'],"albumyear":row['a.year'],"artistname":row['ar.artistname']}
		if 'p.indexid' in row:
			somesong['indexid'] = row['p.indexid']
		if row['m.artistname'] != None and row['s.metaartistid'] != '-1':
			somesong['metaartistid'] = row['s.metaartistid']
			somesong['metaartistname'] = row['m.artistname']
		if getgenres == True:
			somesong['genres'] = self.fillSongGenreHash(row['s.songid'])
		result.append(somesong)

	def fillSongGenreHash(self, songid):
		result = []
		cursor = self.conn.cursor()
		SQL = "SELECT g.genreid, g.genrename FROM genre_data d INNER JOIN genres g ON d.genreid = g.genreid WHERE d.songid = %s"
		cursor.execute(SQL, songid)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result.append({"genreid":row['g.genreid'],"genrename":row['g.genrename']})
		return result

	def topArtists(self, numbertoget):
		result = []
		cursor = self.conn.cursor()
		SQL = "select count(*) as thecount, a.artistname from history h inner join songs s on h.songid = s.songid inner join artists a on a.artistid = s.artistid where h.type = 'p' group by a.artistname order by thecount desc, a.artistname asc limit %s"
		cursor.execute(SQL, numbertoget)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result.append({"artistname":row['a.artistname'],"count":int(row['thecount'])})
		return result

	def topAlbums(self, numbertoget):
		result = []
		cursor = self.conn.cursor()
		SQL = "select count(*) as thecount, al.albumname, a.artistname from history h inner join songs s on h.songid = s.songid inner join albums al on s.albumid = al.albumid inner join artists a on al.artistid = a.artistid where h.type = 'p' group by a.artistname, al.albumname order by thecount desc, a.artistname asc, al.albumname asc limit %s"
		cursor.execute(SQL, numbertoget)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result.append({"artistname":row['a.artistname'],"albumname":row['al.albumname'],"count":int(row['thecount'])})
		return result

	def topSongs(self, numbertoget):
		result = []
		cursor = self.conn.cursor()
		SQL = "select count(*) as thecount, al.albumname, a.artistname, s.songname from history h inner join songs s on h.songid = s.songid inner join albums al on s.albumid = al.albumid inner join artists a on al.artistid = a.artistid where h.type = 'p' group by a.artistname, al.albumname, s.songname order by thecount desc, a.artistname asc, al.albumname asc, s.songname asc limit %s"
		cursor.execute(SQL, numbertoget)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result.append({"artistname":row['a.artistname'],"albumname":row['al.albumname'],"songname":row['s.songname'],"count":int(row['thecount'])})
		return result

	def getStats(self):
		result = {}
		cursor = self.conn.cursor()
		SQL = "select count(*) as numartists from artists"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result["numartists"] = int(row['numartists'])
		SQL = "select count(*) as numalbums from albums"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result["numalbums"] = int(row['numalbums'])
		SQL = "select count(*) as numsongs, sum(filesize) as sumfilesize, sum(songlength) as sumsec, avg(filesize) as avgfilesize, avg(songlength) as avgsec from songs"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result["numsongs"] = int(row['numsongs'])
			result["sumfilesize"] = float(row['sumfilesize'])
			result["sumsec"] = float(row['sumsec'])
			result["avgfilesize"] = float(row['avgfilesize'])
			result["avgsec"] = float(row['avgsec'])
		SQL = "select count(*) as songsplayed from history where type = 'p'"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result["songsplayed"] = int(row['songsplayed'])
		SQL = "select count(*) as songsstarted from history where type = 's'"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			result["songsstarted"] = int(row['songsstarted'])
		return result

	def importCache(self):
		log.debug("funcs", "Database.importCache()")
		result = []
		cursor = self.conn.cursor()
		SQL = "SELECT filename, modified_date FROM songs ORDER BY filename"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
#			if type(row['modified_date']) is not FloatType:
#				row['modified_date'] = 

			result.append({"filename":row['filename'],"modifieddate":row['modified_date']})
		cursor.close()
		return result

	def getmetadata(self, filename):
		log.debug("funcs", "Database.getmetadata(%s)", filename)
		return getTag(filename)
		

	def importNewSongs(self, songs):
		log.debug("funcs", "Database.importNewSongs()")

		cursor = self.import_cursor
		cursor.begin()
		try:
			for song in songs:
				log.debug("import", "Importing song %s as %s", song["filename"], song)
				if "bitrate" in song.keys():
					genreid = -1
					if 'genre' in song.keys():
						genreid = self._getGenre(self.checkBinary(song['genre']))
					artistid = self._getArtist(self.checkBinary(song['artistname']),False)
					metaartistid = -1
					if 'metaartistname' in song.keys():
						metaartistid = self._getArtist(self.checkBinary(song['metaartistname']),True)
					albumid = self._getAlbum(self.checkBinary(song['albumname']), artistid, song['year'])
					songid = self._getNSong(self.checkBinary(song['songname']),artistid,self.checkBinary(song['filename']),song['tracknum'],albumid=albumid,year=song['year'],metaartistid=metaartistid, bitrate=song["bitrate"], songlength=song["songlength"], genreid=genreid)
				else:
					log.debug("import", "Could not get bitrate of song %s.  Assuming bad file.", song["filename"])
		except:
			cursor.rollback()
			raise

		cursor.commit()
		return True


	def _getNSong(self, songname, artistid, filename, tracknum, albumid="", year="", metaartistid=-1, genreid=-1, bitrate=-1, songlength=-1):
		log.debug("funcs", "Database._getNSongs()")
		sid = -1
		songname = string.strip(songname)
		filename = string.strip(filename)
		cursor = self.import_cursor

		statinfo = os.stat(filename)
		now = time.time()

		if tracknum == -1:
			tracknum = 0

		if filename not in self.i_songcache:
			SQL = "insert into songs (songname, artistid, albumid, year, tracknum, filename, filesize, songlength, bitrate, metaartistid, create_date, modified_date, timesplayed, weight, flags) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, 0, 0, 0)"
			cursor.execute(SQL, songname, artistid, albumid, year, tracknum, filename, statinfo.st_size, songlength, bitrate, metaartistid, now, now)
			self.getalbumstatus = True
			sid = cursor.lastrowid
			if genreid != -1:
				SQL = "insert into genre_data(songid, genreid) VALUES (%s, %s)"
				cursor.execute(SQL, sid, genreid)
			self.i_songcache[filename] = sid
		#TODO: Check to see if there are changes
		else:
			sid = self.i_songcache["filename"]
			SQL = "update songs set modified_date = %s, songname = %s, artistid = %s, albumid = %s, year = %s, tracknum = %s, filename = %s, songlength = %s, bitrate = %s, metaartistid = %s, filesize = %s where songid = %s"
			cursor.execute(SQL, now, songname, artistid, albumid, year, tracknum, filename, songlength, bitrate, metaartistid, statinfo.st_size, sid)
			if genreid != -1:
				SQL = "update genre_data set genreid=%s WHERE songid=%s"
				cursor.execute(SQL, genreid, sid)
			self.getalbumstatus = False
		return sid


	def importStart(self):
		log.debug("funcs", "Database.importStart()")
		session = Session()
		self.genrecache = {}
		self.artistcache = {}
		self.albumcache = {}
		self.i_songcache = {}
		#self.cursong = session['xinelib'].createSong()
		#self.cursong.songInit()
		self.import_cursor = self.conn.cursor()
		self.import_cursor.nolock=1
		cursor=self.import_cursor

		SQL = "select artistname,artistid from artists"
		cursor.execute(SQL)		
		for row in cursor.fetchall():
			log.debug("sqlresult", "%s", row)
			self.artistcache[row[0]] = int(row[1])
		SQL = "select artistid,albumname,albumid from albums"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "%s", row)
			self.albumcache[str(row[0])+row[1]] = int(row[2])

		SQL = "select filename,songid from songs"
		cursor.execute(SQL)
		for row in cursor.fetchall():
			log.debug("sqlresult", "%s", row)
			self.i_songcache[row[0]] = int(row[1])

	def importEnd(self):
		log.debug("funcs", "Database.importEnd()")
		cursor = self.import_cursor
		cursor.begin()
		try:
			SQL = "DELETE FROM albums WHERE albumid NOT IN (SELECT albumid FROM songs)"
			cursor.execute(SQL)
			SQL = "DELETE FROM artists WHERE artistid NOT IN (SELECT artistid FROM songs) and artistid NOT IN (SELECT metaartistid as artistid FROM songs)"
			cursor.execute(SQL)
		except:
			cusor.rollback()
			raise

		cursor.commit()
		
		log.debug("import", "Import complete, loading song cache (before %d)", len(self.songcache))
		try:
			self.loadSongCache()
		except:
			log.exception("Got exception trying to upgrade song cache")
		log.debug("import", "Cache update complete. Cache contains %d songs", len(self.songcache))

	def importCancel(self):
		log.debug("funcs", "Database.importCancel()")


	def importUpload(self, filename, songdata):
		log.debug("funcs", "Database.importUpload()")
		log.debug("import", "getting tag info for: %s" % self.checkBinary(filename))
		return getTag(self.checkBinary(filename))

	def importSongs(self, somesong):
		log.debug("funcs", "Database.importSongs()")
		resultmem = {}
		genreid = -1
		if 'genrename' in somesong.keys():
			genreid = self._getGenre(self.checkBinary(somesong['genrename']))
		artistid = self._getArtist(self.checkBinary(somesong['artistname']),False)
		metaartistid = -1
		if 'metaartistname' in somesong.keys():
			metaartistid = self._getArtist(self.checkBinary(somesong['metaartistname']),True)
		albumid = self._getAlbum(self.checkBinary(somesong['albumname']), artistid, somesong['year'])
		songid = self._getSong(self.checkBinary(somesong['songname']),artistid,self.checkBinary(somesong['filename']),somesong['tracknum'],albumid,somesong['year'],metaartistid,genreid=genreid)
		resultmem['genreid'] = genreid
		resultmem['artistid'] = artistid
		resultmem['metaartistid'] = metaartistid
		resultmem['albumid'] = albumid
		resultmem['songid'] = songid
		if self.getgenrestatus != -1:
			resultmem['newgenreid'] =  self.getgenrestatus
		if self.getartiststatus != -1:
			resultmem['newartistid'] =  self.getartiststatus
		if self.getmetaartiststatus != -1:
			resultmem['newmetaartistid'] =  self.getmetaartiststatus
		if self.getalbumstatus != -1:
			resultmem['newalbumid'] = self.getalbumstatus
		if self.getsongstatus != -1:
			resultmem['newsongid'] = self.getsongstatus
		return resultmem

	def importDelete(self, arrayofsongs):
		log.debug("funcs", "Database.importDelete()")
		cursor = self.import_cursor
		result = 0
		for somesong in arrayofsongs:
			somesong = self.checkBinary(somesong)
			SQL=""
			if isinstance(somesong,types.IntType):				
				SQL = "DELETE FROM songs WHERE songid = %s"
			elif isinstance(somesong,types.StringType):
				SQL = "DELETE FROM songs WHERE filename = %s"

			if SQL!="":
				cursor.execute(SQL, somesong)
				result += 1
		return result

	def playlistClear(self, playlistid):
		cursor = self.conn.cursor()
		SQL = "DELETE FROM playlistdata WHERE playlistid = %s"
		cursor.execute(SQL, playlistid)

	def addSongToPlaylist(self, playlistid, songid):
		cursor = self.conn.cursor()
		#SQL = "INSERT INTO playlistdata (playlistid, songid) VALUES (%d, %d)" % (playlistid, songid)
		SQL = "INSERT INTO playlistdata (playlistid, songid, indexid) values (%d,%d,(select count(playlistdataid) from playlistdata where playlistid = %s))"
		cursor.execute(SQL, playlistid, songid, playlistid)

	def removeSongFromPlaylist(self, playlistid, indexid):
		playlistdataid = 0
		cursor = self.conn.cursor()
		SQL = "DELETE from playlistdata where playlistid = %d and indexid = %s"
		cursor.execute(SQL, playlistid, indexid)
		SQL = "UPDATE playlistdata set indexid = indexid - 1 where playlistid = %s and indexid > %s"
		cursor.execute(SQL, playlistid, indexid)

	def moveSongInPlaylist(self, playlistid, index1, index2, swap=False):
		songs = []
		cursor = self.conn.cursor()
		SQL = "SELECT songid from playlistdata where playlistid = %s order by indexid"
		cursor.execute(SQL, playlistid)
		for row in cursor.fetchall():
			log.debug("sqlresult", "Row: %s", row)
			songs.append(row['songid'])
		if index1 > -1 and index1 < len(songs) and index2 > -1 and index2 < len(songs):
			if swap == False:
				tmp = songs[index1]
				songs.pop(index1)
				songs.insert(index2, tmp)
			else:
				tmp = songs[index1]
				songs[index1] = songs[index2]
				songs[index2] = tmp 
			self.playlistClear(playlistid)
			for i in songs:
				self.addSongToPlaylist(playlistid, i)

	def _getGenre(self, genrename):
		log.debug("funcs", "Database._getGenre()")
		gid = -1
		genrename = string.strip(genrename)
		cursor = self.import_cursor
		if genrename not in self.genrecache:
			SQL = "select genreid from genres where genrename = %s"
			cursor.execute(SQL, genrename)
			for row in cursor.fetchall():
				log.debug("sqlresult", "%s", row)
				gid = row['genreid']
			if gid == -1:
				now = time.time()
				SQL = "insert into genres (genrename, create_date) values (%s, %s)"
				cursor.execute(SQL, genrename, now)
				self.getgenrestatus = True
				SQL = "select genreid from genres where genrename = %s"
				cursor.execute(SQL, genrename)
				for row in cursor.fetchall():
					log.debug("sqlresult", "%s", row)
					gid = row['genreid']
				else:
					SQL = "update genres set genrename = %s, modified_date = %s where genreid=%s"
					cursor.execute(SQL, genrename, now, gid)
				self.getgenrestatus = False
			self.genrecache[genrename] = gid
		else:
			self.getgenrestatus = -1
			gid = self.genrecache[genrename]
		return gid

	def _getArtist(self, artistname, metaartist=False):
		log.debug("funcs", "Database._getArtist()")
		aid = -1
		artistname = string.strip(artistname)
		cursor = self.import_cursor
		#See if this artist is already in the cache
		if artistname not in self.artistcache:
			#SQL = "select artistid from artists where artistname = %s"
			#cursor.execute(SQL, artistname)
			#for row in cursor.fetchall():
			#	log.debug("sqlresult", "Row: %s", row)
			#	aid = row['artistid']
			now = time.time()
			try:
				metaartist = int(metaartist)
			except:
				metaartist = 0

			if aid == -1:
				SQL = "insert into artists (artistname, metaflag, create_date, modified_date) VALUES (%s, %s, %s, %s)"
				cursor.execute(SQL, artistname, int(metaartist), now, now)
				self.getartiststatus = True
				aid = cursor.lastrowid
			#Not needed until we have genres and/or metaartists
			else:
				SQL = "update artists set metaflag = %s, modified_date = %s where artistid = %s"
				cursor.execute(SQL, metaartist, now, aid)
				self.getartiststatus = False
			self.artistcache[artistname] = aid
		else:
			self.getartiststatus = -1
			aid = self.artistcache[artistname]
		return aid

	def _getAlbum(self, albumname, artistid, year):
		tid = -1
		albumname = string.strip(albumname)
		cursor = self.import_cursor
		#See if this album is already in the cache
		if str(str(artistid) + albumname) not in self.albumcache:
			#SQL = "select albumid from albums where albumname = %s"
			#cursor.execute(SQL, albumname)
			#for row in cursor.fetchall():
			#	log.debug("sqlresult", "Row: %s", row)
			#	tid = row['albumid']
			now=time.time()
			if tid == -1:
				SQL = "insert into albums (albumname, artistid, year, create_date, modified_date) VALUES (%s, %s, %s, %s, %s)"
				cursor.execute(SQL, albumname, artistid, year, now, now)
				self.getalbumstatus = True
				tid = cursor.lastrowid
			#TODO: Check to see if there are changes
			else:
				#TODO: Have to add genre code
				SQL = "update albums set modified_date = %s, year = %s, artistid = %s where albumid = %s"
				cursor.execute(SQL, now, year, artistid, tid)
				self.getalbumstatus = False
			self.albumcache[str(artistid) + albumname] = tid
		else:
			self.getalbumstatus = -1
			tid = self.albumcache[str(artistid) + albumname]
		return tid

	def _getSong(self, songname, artistid, filename, tracknum, albumid="", year="", metaartistid=-1, genreid=-1):
		sid = -1
		songname = string.strip(songname)
		filename = string.strip(filename)
		cursor = self.import_cursor
		#SQL = "select songid from songs where filename = %s"
		#cursor.execute(SQL, filename)
		#for row in cursor.fetchall():
		#	log.debug("sqlresult", "Row: %s", row)
		#	sid = row['songid']

		#metadata = self.cursong.getMetaData()
		metadata = {}
		try:
			#Jef 07/30/2003: Not sure why but metadata=metadata.id3.getTag(filename) isnt working
			metadata = getTag(filename)
			log.debug("import", "Metadata %s", metadata)
		except:
			log.debug("import", "No metadata for %s", filename)
		if "bitrate" not in metadata or "songlength" not in metadata:
			pass
			#print "before set filename"
			#self.cursong.songint.filename = filename
			#print "before open"
			#self.cursong.songOpen()
			#print "before metadata"
			#metadata = self.cursong.getMetaData()
			#self.cursong.songClose()
		#print "after metadata"
		statinfo = os.stat(filename)
		songlength = 0
		if metadata['songlength'] is not None and str(metadata['songlength']) != 'inf':
			songlength = metadata['songlength']
		now = time.time()
		artistid = int(artistid)
		albumid = int(albumid)
		year = int(year)

		if filename not in self.i_songcache:
			SQL = "insert into songs (songname, artistid, albumid, year, tracknum, filename, filesize, songlength, bitrate, metaartistid, create_date, modified_date, timesplayed, weight, flags) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, 0, 0, 0)"
			cursor.execute(SQL, songname, artistid, albumid, year, tracknum, filename, statinfo.st_size, songlength, metadata['bitrate'], metaartistid, now, now)
			self.getalbumstatus = True
			sid = cursor.lastrowid
			if genreid > -1:
				SQL = "insert into genre_data(songid, genreid, create_date, modified_date) VALUES (%s, %s, %s, %s)"
				cursor.execute(SQL, sid, genreid, now, now)
			self.i_songcache[filename] = sid
		#TODO: Check to see if there are changes
		else:
			SQL = "update songs set modified_date = %s, songname = %s, artistid = %s, albumid = %s, year = %s, tracknum = %s, filename = %s, songlength = %s, bitrate = %s, metaartistid = %s, filesize = %s where songid = %s"
			cursor.execute(SQL, now, songname, artistid, albumid, year, tracknum, filename, metadata['songlength'], metadata['bitrate'], metaartistid, statinfo.st_size, sid)
			if genreid > -1:
				SQL = "update genre_data set genreid=%s, modified_date=%s WHERE songid=%s"
				cursor.execute(SQL, genreid, now, sid)
			self.getalbumstatus = False
		return sid


	def checkBinary(self, datatocheck):
		return UTFstring.decode(datatocheck)

	def getCDDB(self, device):
		tags = getCDDB(device)
		return tags

	def pyrip(self, tags, filenames, device):
		session = Session()
		pyrip (device, session['cfg'], tags, filenames)
		return 0

	def pyrip_update (self):
		ret = pyrip_update()
		if ret['done'] == 1:
			print "Importing files:"
			print ret
			session = Session ()
			session['cmdint'].db.importstart()
			i = 1
			for file in ret['filenames_c']:
				tag = session['cmdint'].db.importupload(file)
				tag['filename'] = file
				tag['metaartistname'] = ''
				tag['tracknum'] = i
				for key in tag:
					tmp = UTFstring.encode (tag[key])
					tag[key] = tmp
				session['cmdint'].db.importsongs(tag)
				i = i + 1
			session['cmdint'].db.importend()
			print "Done importing"
		return ret

# vim:ts=8 sw=8 noet

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

import Database
import os
#import util.Logger
from boss3.util import bosslog
from boss3.util.Session import *

log = bosslog.getLogger()

class DatabaseSchema:

	version = 6
	filename = ""
	schema = """
------------------------------------------------------------
--Create the tables
------------------------------------------------------------

CREATE TABLE version (
	versionnumber	INT
);

CREATE TABLE artists (
	artistid	INTEGER PRIMARY KEY,
	artistname	VARCHAR(100),
	metaflag	INT,
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
);

CREATE TABLE albums (
	albumid		INTEGER PRIMARY KEY,
	artistid	INT,
	albumname	VARCHAR(100),
	year		INT,
	userid		INT,
	frontcover	VARCHAR(255),
	backcover	VARCHAR(255),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
);

CREATE TABLE songs (
	songid		INTEGER PRIMARY KEY,
	artistid	INT,
	metaartistid	INT,
	albumid		INT,
	tracknum	INT,
	songname	VARCHAR(255),
	filename	VARCHAR(255),
	year		INT,
	filesize	INT,
	songlength	INT,
	bitrate		INT,
	timesplayed	INT,
	flags		INT,
	composer	VARCHAR(255),
	weight		NUMERIC(9,12),
	trm		VARCHAR(255),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
);

CREATE TABLE users (
	userid		INTEGER PRIMARY KEY,
	username	VARCHAR(20),
	password	VARCHAR(20),
	authlevel	INT,
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
);

CREATE TABLE genres (
	genreid		INTEGER PRIMARY KEY,
	genrename	VARCHAR(20),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
);

CREATE TABLE genre_data (
	genredataid	INTEGER PRIMARY KEY,
	genreid		INT,
	songid		INT
);

CREATE TABLE history (
	userid		INT,
	songid		INT,
	time		INT,
	type		CHAR(1)
);

CREATE TABLE currentstate (
	queueindex	INT,
	playlistid	INT,
	songid		INT,
	shuffle		INT
);

CREATE TABLE queue (
	indexid		INTEGER PRIMARY KEY,
	songid		INT
);

CREATE TABLE playlists (
	playlistid	INTEGER PRIMARY KEY,
	name		VARCHAR(30),
	userid		INT,
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
);

CREATE TABLE playlistdata (
	playlistdataid	INTEGER PRIMARY KEY,
	playlistid	INT,
	songid		INT,
	indexid		INT
);

------------------------------------------------------------
--Make some indexes
------------------------------------------------------------

CREATE INDEX artists_artistname_idx ON artists(artistname);
CREATE INDEX artists_artistid_idx ON artists(artistid);

CREATE INDEX albums_albumid_idx ON albums(albumid);
CREATE INDEX albums_albumname_idx ON albums(albumname);

CREATE INDEX songs_songid_idx ON songs(songid);
CREATE INDEX songs_artistid_idx ON songs(artistid);
CREATE INDEX songs_albumid_idx ON songs(albumid);
CREATE INDEX songs_songname_idx ON songs(songname);
CREATE INDEX songs_metaartistid_idx ON songs(metaartistid);

CREATE INDEX genres_genreid_idx ON genres(genreid);
CREATE INDEX genre_data_genreid_idx ON genre_data(genreid);
CREATE INDEX genre_data_songid_idx ON genre_data(songid);

------------------------------------------------------------
--Set some default values
------------------------------------------------------------

INSERT INTO VERSION (versionnumber) VALUES (6);
INSERT INTO USERS (username, password, authlevel) VALUES ('blah', 'blah', (256+128+64+32+16+8+4+2+1));
"""
	def create(self, filename=None):

		session = Session()

		if (filename != None):
			self.filename = filename
		elif (os.getenv("BOSS_DB") != None):
			self.filename = os.getenv("BOSS_DB")
		else:
			if session['userdir']:
				self.filename = session['userdir'] + "/.bossogg/bossogg.db"
			elif (os.geteuid() == 0):
				prefix = "/var/lib/bossogg"
				self.filename = prefix + "/bossogg.db"
			else:
				self.filename = os.path.expanduser("~/.bossogg/bossogg.db")

		log.debug("sql", "Using database file:" + self.filename)
		if (os.path.exists(self.filename) == False):
			log.debug("sql", "Database Does Not Exist... creating")
			head, tail = os.path.split(self.filename)
			if (os.path.exists(head) == False):
				os.makedirs(head)
			dbh = Database.Database()
			dbh.dbname = self.filename
			dbh.connect(True)
			dbh.runScript(self.schema)
			dbh.disconnect()
		else: #Check version and update
			dbh = Database.Database()
			dbh.dbname = self.filename
			dbh.connect(True)
			while (dbh.getSchemaVersion() < self.version):
				self.updateSchema(dbh)
			dbh.disconnect()

		return self.filename
	
	def updateSchema(self, dbh):
		curver = dbh.getSchemaVersion()
		if curver == 1:
			try:
				dbh.runScript("DROP TABLE playlistdata_tmpstuff")
			except Exception:
				pass
			dbh.runScript("""CREATE TABLE playlistdata_tmpstuff (
	playlistdataid	INTEGER PRIMARY KEY,
	playlistid	INT,
	songid		INT
)""")
			dbh.runScript("INSERT INTO playlistdata_tmpstuff SELECT * from playlistdata")
			dbh.runScript("DROP TABLE playlistdata")
			dbh.runScript("""CREATE TABLE playlistdata (
	playlistdataid	INTEGER PRIMARY KEY,
	playlistid	INT,
	songid		INT,
	indexid		INT
)""")
			dbh.runScript("INSERT INTO playlistdata (playlistdataid, playlistid, songid) SELECT playlistdataid, playlistid, songid from playlistdata_tmpstuff")
			dbh.runScript("DROP TABLE playlistdata_tmpstuff")
			dbh.runScript("UPDATE version set versionnumber = 2")
		if curver == 2:
			try:
				dbh.runScript("CREATE INDEX songs_metaartistid_idx ON songs(metaartistid)")
			except Exception:
				pass
			dbh.runScript("UPDATE version set versionnumber = 3")
		if curver == 3:
			dbh.runScript("""CREATE TABLE genres (
	genreid		INTEGER PRIMARY KEY,
	genrename	VARCHAR(20),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
)""")
			dbh.runScript("""CREATE TABLE genre_data (
	genredataid	INTEGER PRIMARY KEY,
	genreid		INT,
	songid		INT
)""")
			try:
				dbh.runScript("CREATE INDEX genres_genreid_idx ON genres(genreid)")
				dbh.runScript("CREATE INDEX genre_data_genreid_idx ON genre_data(genreid)")
				dbh.runScript("CREATE INDEX genre_data_songid_idx ON genre_data(songid)")
			except Exception:
				pass
			dbh.runScript("UPDATE version set versionnumber = 4")
		if curver == 4:
			try:
				dbh.runScript("DROP TABLE songs_tmpstuff")
			except Exception:
				pass
			dbh.runScript("""CREATE TABLE songs_tmpstuff (
	songid		INTEGER PRIMARY KEY,
	artistid	INT,
	metaartistid	INT,
	albumid		INT,
	tracknum	INT,
	songname	VARCHAR(255),
	filename	VARCHAR(255),
	year		INT,
	filesize	INT,
	songlength	INT,
	bitrate		INT,
	timesplayed	INT,
	flags		INT,
	weight		NUMERIC(9,12),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
)""")
			dbh.runScript("INSERT INTO songs_tmpstuff SELECT * from songs")
			dbh.runScript("DROP TABLE songs")
			dbh.runScript("""CREATE TABLE songs (
	songid		INTEGER PRIMARY KEY,
	artistid	INT,
	metaartistid	INT,
	albumid		INT,
	tracknum	INT,
	songname	VARCHAR(255),
	filename	VARCHAR(255),
	year		INT,
	filesize	INT,
	songlength	INT,
	bitrate		INT,
	timesplayed	INT,
	flags		INT,
	composer	VARCHAR(255),
	weight		NUMERIC(9,12),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
)""")
			dbh.runScript("INSERT INTO songs (songid, artistid, metaartistid, albumid, tracknum, songname, filename, year, filesize, songlength, bitrate, timesplayed, flags, weight, create_date, modified_date) SELECT songid, artistid, metaartistid, albumid, tracknum, songname, filename, year, filesize, songlength, bitrate, timesplayed, flags, weight, create_date, modified_date from songs_tmpstuff")
			dbh.runScript("DROP TABLE songs_tmpstuff")
			dbh.runScript("UPDATE version set versionnumber = 5")
			dbh.runScript("VACUUM")
		if curver == 5:
			try:
				dbh.runScript("DROP TABLE albums_tmpstuff")
				dbh.runScript("DROP TABLE songs_tmpstuff")
			except Exception:
				pass
			dbh.runScript("""CREATE TABLE albums_tmpstuff (
	albumid		INTEGER PRIMARY KEY,
	artistid	INT,
	albumname	VARCHAR(100),
	year		INT,
	userid		INT,
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
)""")
			dbh.runScript("INSERT INTO albums_tmpstuff SELECT * from albums")
			dbh.runScript("DROP TABLE albums")
			dbh.runScript("""CREATE TABLE albums (
	albumid		INTEGER PRIMARY KEY,
	artistid	INT,
	albumname	VARCHAR(100),
	year		INT,
	userid		INT,
	frontcover	VARCHAR(255),
	backcover	VARCHAR(255),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
)""")
			dbh.runScript("INSERT INTO albums (albumid, artistid, albumname, year, userid, create_date, modified_date) SELECT albumid, artistid, albumname, year, userid, create_date, modified_date FROM albums_tmpstuff")
			dbh.runScript("DROP TABLE albums_tmpstuff")
			dbh.runScript("""CREATE TABLE songs_tmpstuff (
	songid		INTEGER PRIMARY KEY,
	artistid	INT,
	metaartistid	INT,
	albumid		INT,
	tracknum	INT,
	songname	VARCHAR(255),
	filename	VARCHAR(255),
	year		INT,
	filesize	INT,
	songlength	INT,
	bitrate		INT,
	timesplayed	INT,
	flags		INT,
	composer	VARCHAR(255),
	weight		NUMERIC(9,12),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
)""")
			dbh.runScript("INSERT INTO songs_tmpstuff SELECT * from songs")
			dbh.runScript("DROP TABLE songs")
			dbh.runScript("""CREATE TABLE songs (
	songid		INTEGER PRIMARY KEY,
	artistid	INT,
	metaartistid	INT,
	albumid		INT,
	tracknum	INT,
	songname	VARCHAR(255),
	filename	VARCHAR(255),
	year		INT,
	filesize	INT,
	songlength	INT,
	bitrate		INT,
	timesplayed	INT,
	flags		INT,
	composer	VARCHAR(255),
	weight		NUMERIC(9,12),
	trm		VARCHAR(255),
	create_date	TIMESTAMP,
	modified_date	TIMESTAMP
)""")
			dbh.runScript("INSERT INTO songs (songid, artistid, metaartistid, albumid, tracknum, songname, filename, year, filesize, songlength, bitrate, timesplayed, flags, weight, create_date, modified_date) SELECT songid, artistid, metaartistid, albumid, tracknum, songname, filename, year, filesize, songlength, bitrate, timesplayed, flags, weight, create_date, modified_date from songs_tmpstuff")
			dbh.runScript("DROP TABLE songs_tmpstuff")
			dbh.runScript("UPDATE version set versionnumber = 6")
			dbh.runScript("CREATE INDEX songs_albumid_idx ON songs(albumid)")
			dbh.runScript("CREATE INDEX songs_artistid_idx ON songs(artistid)")
			dbh.runScript("VACUUM")
# vim:ts=8 sw=8 noet

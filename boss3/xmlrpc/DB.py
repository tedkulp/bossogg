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
from boss3.util import UTFstring
from types import DictType, ListType, StringType, UnicodeType
from boss3.util import bosslog

log = bosslog.getLogger()

class DB:
	"""
	DB realm.  This will handle all aspects of inserting information
	into the datbase.  This is mostly used for, but not limited to,
	the import procedure.  This realm will be locked when certain
	intensive database operations are running.


	db("importstart"): Start the import prodecure.  This will clear
	some internal caches and lock the database from any other use.

	Parameters:
	* None

	Returns:
	* 1 - int


	db("importend"): Complete the import process.  This will reopen
	the database lock and commit the transaction.

	Parameters:
	* None

	Returns:
	* 1 - int


	db("importcancel"): Abort the import process.  This will reopen
	the database lock and rollback the transaction.

	Parameters:
	* None

	Returns:
	* 1 - int


	db("importcache"): Get a list of all of the songs in the database.
	This is used by import clients to tell if certain songs should be
	deleting from the database because they no longer exist in the
	filesystem.

	Parameters:
	* None

	Returns:
	* Array
	  * Struct
	    * filename - string(base64)
	    * modifieddate - float (number of seconds since the epoch)


	db("upload"): Return the known metadata from the filename given.
	This will later be used to upload songs into the database from
	a client and have the server then perform the metadata lookup
	from the given file.

	Parameters:
	* filename - string(base64)
	* filedata - base64(not used yet)

	Returns:
	* Struct
	  * artistname - string(base64)
	  * songname - string(base64)
	  * albumname - string(base64)
	  * genre - string(base64)
	  * year - int
	  * track - int
	  * bitrate - int
	  * frequency - int
	  * songlength - float


	db("importsong"): Insert the actual song with the given data into
	the database.  It returns values that could be of use to the client
	including the artist, album and song ids and whether or not this this
	a new artist,album,song or just an update to something at is already
	in the database.

	Parameters:
	* Struct
	  * artistname - string(base64)
	  * metaartistname - string(base64)
	  * albumname - string(base64)
	  * year - int
	  * songname - string(base64)
	  * filename - string(base64)
	  * tracknum - int

	 Returns:
	 * Struct
	   * artistid - int
	   * albumid - int
	   * songid - int
	   * newartistid - int(1 or 0)
	   * newalbumid - int(1 or 0)
	   * newsongid - int(1 or 0)


	 db("importdelete"): Removes the given song from the database.
	 It returns the number of successfully removed songs.

	 Parameters:
	 * Array
	   * filename - string(base64)
	   -- or --
	   * songid - int

	 Returns:
	 * numsongs - int
	"""

	def handleRequest(self, cmd, argstuple):

		session = Session()

		log.debug("funcs", "handleRequest(%s, %s)", cmd, argstuple)
		args = []
		for i in argstuple:
			args.append(i)

		try:
			if (session.hasKey('cmdint')):
				cmdint = session['cmdint']

				if (cmd == "importstart"):
					cmdint.db.importstart()
					return 1
				elif (cmd == "importend"):
					cmdint.db.importend()
					return 1
				elif (cmd == "importcancel"):
					cmdint.db.importcancel()
					return 1
				elif (cmd == "importsong"):
					if len(args) > 0:
						if type(args[0]) is DictType:
							tmp = args[0]
							for i in tmp.keys():
								tmp[i] = UTFstring.decode(tmp[i])
							#print "tmp:%s" % str(tmp)
							return cmdint.db.importsongs(tmp)
						else:
							#It's not a valid param
							pass
					else:
						#It's a bad command
						pass
				elif (cmd == "importdelete"):
					if len(args) > 0:
						if isinstance(args[0], ListType):
							tmp = args[0]
							#for i in tmp:
							#	if type(tmp[i]) is xmlrpclib.Binary:
							#		tmp[i] = UTFstring.decode(tmp[i])
							return cmdint.db.importdelete(tmp)
						else:
							#It's not a valid param
							pass
					else:
						#It's a bad command
						pass
				elif (cmd == "importcache"):
					cache = cmdint.db.importcache()
					for i in cache:
						tmp= UTFstring.encode(i['filename'])
						i['filename'] = tmp
					return cache
				elif cmd == "upload":
					if len(args) > 0:
						thehash = cmdint.db.importupload(UTFstring.decode(args[0]))
						for i in thehash:
							if isinstance(thehash[i], UnicodeType) or isinstance(thehash[i], StringType):
								tmp = UTFstring.encode(thehash[i])
								thehash[i] = tmp
						return thehash
					else:
						#TODO: Make an error fault
						pass
				elif cmd == "getCDDB":
					if len(args) > 0:
						tags = cmdint.db.getCDDB(args[0])
						return tags
					else:
						pass
				elif cmd == "pyrip":
					if len(args) > 0:
						return cmdint.db.pyrip (args[0], args[1], args[2])
					else:
						pass
				elif cmd == "pyrip_update":
					return cmdint.db.pyrip_update ()
		except:
			log.exception("Exception while calling XML function")
			raise


# vim:ts=8 sw=8 noet

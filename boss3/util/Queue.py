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

from boss3.util.Session import *
from boss3.bossexceptions import EndOfQueueException
import bosslog
import md5
import random
import time
import types

log = bosslog.getLogger()

class CustomQueue:
	def __init__(self, manager, dbh, parent):
		self.items = []
		self.dbh = dbh
		self.parent = parent
		self.manager = manager
		self.currentindex = 0
	def add(self, item):
		self.items.append(item)
	def remove(self, item):
		if self.manager.usingendofqueue or ((item != self.currentindex) and ( item >= 0) and (item < len(self.items))):
			self.items.pop(item)
			if (item < self.currentindex):
				self.currentindex=self.currentindex-1
	def clear(self):
		self.items=[]
		self.currentindex=0
		self.manager.usingendofqueue = 1
	def shuffle(self):
		random.shuffle(self.items)
		print self.items
	def getSongIDs(self):
		result = []
		for i in self.items:
			if 'songid' in i:
				result.append(int(i['songid']))
		return result
	def getmd5sum(self):
		md5sum = md5.new()
		for i in self.items:
			if 'songid' in i:
				md5sum.update(str(i['songid']) + " ")
		return md5sum.hexdigest()
	def jump(self,index):
		if isinstance(index, types.StringType):
			index = int(index)
		if index >= 0 and index < len(self.items):
			self.currentindex = index
	def getCurrentSong(self):
		if 0 <= self.currentindex < len(self.items):
			return self.items[self.currentindex]
		else:
			return None
	def next(self):
		self.currentindex += 1
		if self.currentindex >= len(self.items):
			self.currentindex = len(self.items)
			return None
		else:
			return self.items[self.currentindex]
	def prev(self):
		if self.currentindex > 0:
			self.currentindex=self.currentindex-1
	def move(self,oldindex,newindex):
		if (newindex > -1) and ( newindex < len(self.items)):  
			if (oldindex > -1) and ( oldindex < len(self.items)):  
				tmp=self.items[newindex]
				#do a check for outofbounds
				self.items[newindex]=self.items[oldindex]
				self.items[oldindex]=tmp

class UserEditableQueue(CustomQueue):
	def __init__(self, manager, dbh, parent):
		# The standard queue interface does exactly the right thing for
		# a user editable queue.
		CustomQueue.__init__(self, manager, dbh, parent)

class AllRandomQueue(CustomQueue):
	def __init__(self, manager, dbh, parent):
		CustomQueue.__init__(self, manager, dbh, parent)
		songs = dbh.listSongs(getgenres=False)
		for song in songs:
			self.items.append(song)
		random.shuffle(self.items)
	# don't allow user to edit this queue; override the editing methods
	def add(self, item):
		pass
	def remove(self, item):
		pass
	def clear(self):
		pass
	def move(self,oldindex,newindex):
		pass

class StopPlayingQueue(CustomQueue):
	def __init__(self, manager, dbh, parent):
		CustomQueue.__init__(self, manager, dbh, parent)
	# don't allow user to edit this queue; override the editing methods
	def add(self, item):
		pass
	def remove(self, item):
		pass
	def clear(self):
		pass
	def move(self,oldindex,newindex):
		pass
	def next(self):
		raise EndOfQueueException.EndOfQueueException("Reached end of main queue; stopping")
	def getCurrentSong(self):
		return {"songid": 1, "songlength": 0, "filename": "", "timesplayed": 0}

class RepeatQueue(CustomQueue):
	def __init__(self, manager, dbh, parent):
		CustomQueue.__init__(self, manager, dbh, parent)
	# don't allow user to edit this queue; override the editing methods
	def add(self, item):
		pass
	def remove(self, item):
		pass
	def clear(self):
		pass
	def move(self,oldindex,newindex):
		pass
	def next(self):
		self.manager.setCurrentIndex(0)
		self.manager.usingendofqueue = 0
		return self.manager.getCurrentSong()["songid"]
	def getCurrentSong(self):
		return None

class AllStraightQueue(CustomQueue):
	def __init__(self, manager, dbh, parent):
		CustomQueue.__init__(self, manager, dbh, parent)
		songs = dbh.listSongs(getgenres=False)
		log.debug("queue", "Called listsongs and got %s", songs)
		for song in songs:
			self.items.append(song)
	# don't allow user to edit this queue; override the editing methods
	def add(self, item):
		pass
	def remove(self, item):
		pass
	def clear(self):
		pass
	def move(self,oldindex,newindex):
		pass

class PlaylistQueue(CustomQueue):
	def __init__(self, manager, dbh, parent):
		CustomQueue.__init__(self, manager, dbh, parent)
		playlist = manager.session["endofqueueparam"]
		self.items = dbh.listSongs(getgenres=False,playlistid=playlist)
	# don't allow user to edit this queue; override the editing methods
	def add(self, item):
		pass
	def remove(self, item):
		pass
	def clear(self):
		pass
	def move(self,oldindex,newindex):
		pass

class QueueManager:
	eoqNameToClass = {"allshuffle": AllRandomQueue,
			  "allstraight": AllStraightQueue,
			  "repeatqueue": RepeatQueue,
			  "playlist": PlaylistQueue,
			  "stop": StopPlayingQueue }
	def __init__(self, dbh, parent):
		self.songqueue = UserEditableQueue(self, dbh, parent)
		self.endofqueue = AllStraightQueue(self, dbh, parent)
		self.dbh = dbh
		self.parent = parent
		self.session = Session()
		self.currenteoqtype = None
		self.usingendofqueue = 0

	def updateEndOfQueue(self):
		log.debug("funcs", "QueueManager.updateEndOfQueue()")
		log.debug("queue", "UEOQ: currenteoqtype: %s sess: %s", self.currenteoqtype, self.session["cfg"].get("endofqueue"))
		if self.session.hasKey("endofqueue"):
			log.debug("queue", "endofqueue set")
			if self.session["endofqueue"] != self.currenteoqtype:
				self.currenteoqtype = self.session["endofqueue"]
				self.endofqueue = self.eoqNameToClass[self.currenteoqtype](self, self.dbh, self.parent)
		else:
			if self.session["cfg"].get("endofqueue") != self.currenteoqtype:
				self.currenteoqtype = self.session["cfg"].get("endofqueue")
				self.endofqueue = self.eoqNameToClass[self.currenteoqtype](self, self.dbh, self.parent)
		log.debug("queue", "End of Queue type is (%s) %s", self.currenteoqtype, self.endofqueue)

	def add(self, item):
		return self.songqueue.add(item)
	def remove(self, item):
		return self.songqueue.remove(item)
	def clear(self):
		return self.songqueue.clear()
	def shuffle(self):
		return self.songqueue.shuffle()
	def getSongIDs(self):
		return self.songqueue.getSongIDs()
	def getmd5sum(self):
		return self.songqueue.getmd5sum()
	def jump(self,index):
		self.usingendofqueue = 0
		return self.songqueue.jump(index)
	def getCurrentSong(self):
		log.debug("funcs", "QueueManager.getCurrentSong()")
		if not self.usingendofqueue:
			temp = self.songqueue.getCurrentSong()
			log.debug("queue", "endofqueue not in use. checking songqueue and got %s", temp)
			if temp is not None: return temp
		log.debug("queue", "Updating endofqueue and using it")
		self.updateEndOfQueue()
		self.usingendofqueue = 1
		temp = self.endofqueue.getCurrentSong()
		log.debug("queue", "Tried to get song and got %s", temp)
		if temp is not None: return temp
		self.endofqueue.currentindex = 0
		temp = self.endofqueue.getCurrentSong()
		log.debug("queue", "Tried to get song again and got %s", temp)
		if temp is not None: return temp
		if len(self.songqueue.items) > 0:
			return self.songqueue.items[0]
		else:
			return None
	def next(self):
		log.debug("funcs", "QueueManager.next()")
		# try real queue first.  if we don't get a valid song, we use
		# the endofqueue queue.  if that doesn't work, reset the
		# endofqueue index and try again.
		if self.usingendofqueue:
			temp = self.songqueue.getCurrentSong()
			# temp will really be the song we want next; it was
			# added to the queue while we were playing the
			# endofqueue queue.
			if temp is not None:
				self.usingendofqueue = 0
				return temp
		temp = self.songqueue.next()
		if temp is not None: return temp
		self.usingendofqueue = 1
		self.updateEndOfQueue()
		temp = self.endofqueue.next()
		if temp is not None: return temp
		# as a last resort, go back to the start of the endofqueue queue.
		self.endofqueue.currentindex = -1
		return self.endofqueue.next()
	def prev(self):
		return self.songqueue.prev(self)
	def move(self,oldindex,newindex):
		return self.songqueue.move(oldindex, newindex)
	def getCurrentIndex(self):
		if self.usingendofqueue:
			return -1
		return self.songqueue.currentindex
	def setCurrentIndex(self, index):
		if index < 0: index=len(self.songqueue.getSongIDs())
		self.songqueue.currentindex = index


# vim:ts=8 sw=8 noet

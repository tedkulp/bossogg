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
#import gstsong
from bossao2 import bossao
#import xinesong.xinesong
import boss3.metadata.id3
import sys
import threading
import time
import Queue
import util.Queue
#import Song
#import SongLib
import string
from util.Session import *
from util import bosslog
from bossexceptions import *

log = bosslog.getLogger()

class Player(threading.Thread):

	dbh = None
	shutdown = 0
	cmdqueue = None
	state = "OK"
	sink = "oss"
	session = None
	cursong = None
	songqueue = None
	status = "PLAYING"
	songid = -1
	totaltime = -1
	playedtime = -1
	playedpercentage = -1

	def __init__(self, dbh):
		threading.Thread.__init__(self)
		self.dbh = dbh
		self.cmdqueue = Queue.Queue(25)
		self.session = Session()
		self.songqueue = util.Queue.QueueManager(dbh, self)

	def _pop_cmd(self):
		try:
			return self.cmdqueue.get(0)
		except Exception:
			return None

	def push_cmd(self, cmd):
		try:
			self.cmdqueue.put(cmd)
			log.debug("audio", "Command %s pushed into queue", cmd)
			return True
		except Exception:
			return False

	def queueMove(self, oldindex, newindex):
		self.songqueue.move(oldindex, newindex)

	def queueRemove(self, index):
		log.debug("audio", "Removing index %d", index)
		self.songqueue.remove(index)

	def queueShuffle(self):
		log.debug("audio", "Shuffling Queue")
		self.songqueue.shuffle()

	def queueClear(self):
		self.songqueue.clear()

	def queueSong(self, songid):
		self.songqueue.add(songid)

	def setVolume(self, volume):
		if self.mixer is not None:
			print "vol net yet implemented"
			#bossao.bossao_setvol(self.mixer, volume)

	def getStatus(self):
		result = {}
		result['queuemd5sum'] = self.songqueue.getmd5sum()
		result['status'] = self.state
		if self.mixer is not None:
			result['volume'] = bossao.bossao_getvol(self.mixer)
		else:
			result['volume'] = 0
		#if self.songqueue.usingqueue == True:
		result['queueindex'] = int(self.songqueue.getCurrentIndex())
		#else:
			#result['queueindex'] = -1
		if self.state != "STOP":
			result['songid'] = int(self.songid)
			result['playedpercentage'] = float(self.playedpercentage)
			result['timeplayed'] = float(self.playedtime)
			result['songlength'] = float(self.totaltime)
		return result

	def updateTimeInfo(self):
		self.playedtime = bossao.bossao_time_current()
		self.totaltime = bossao.bossao_time_total()
		if self.totaltime > 0:
			self.playedpercentage = self.playedtime / self.totaltime * 100
			if self.playedflag == 1 and self.playedpercentage > 75:
				self.dbh.incrementTimesPlayed(self.songid);
				self.playedflag = 0

		if bossao.bossao_finished() == 1:
			#self.state = "NEXT"
			self.handleNextCommand()

	def _update_status(self):
		if self.session.hasKey('shutdown'):
			self.shutdown = 1
			self.state = "SHUTDOWN"
		else:
			if self.state != "STOPPED" and self.state != "SHUTDOWN":
				self.updateTimeInfo()

	def initializeao(self):
		self.songdetails = self.songqueue.getCurrentSong()
		#opendev = bossao.bossao_start(self.lib, self.configfile)
		opendev = 0;
		if opendev is not 0:
			log.debug("audio", "Audio device not opened.  Closing...")
			self.session['shutdown'] = 1
			self.shutdown = 1
			self.state = "SHUTDOWN"
		else:
			time.sleep(.1)
			#log.debug("audio", "Using AO device: %s", bossao2.bossao_driver_name(self.lib))
			self.aoinitialized = 1
			self.shutdown = 0
			self.state = "STOPPED"

	def startPlaying(self):
		log.debug("funcs", "Player.startPlaying()")
		self.songdetails = self.songqueue.getCurrentSong()		
		if self.songdetails is not None:
			log.debug("audio", "Calling bossao_play with filename %s", self.songdetails['filename'])
			bossao.bossao_stop ()
			bossao.bossao_play(self.songdetails['filename'])
			log.debug("audio", "bossao_play returned")
			self.playedflag = 1
			self.songid = self.songdetails["songid"]
			self.state = "PLAYING"
			self.dbh.incrementTimesStarted(self.songid);
			self.sleeplength = .1

	def handleStopCommand(self):
		bossao.bossao_stop ()
		#bossao.bossao_shutdown(self.lib)
		self.state = "STOPPED"
		self.sleeplength = 1

	def handlePlayCommand(self):
		if self.state != "STOPPED":
			self.state = "PLAYING"
			bossao.bossao_unpause()
		else:
			self.startPlaying()

	def handlePauseCommand(self):
		if self.state != "STOPPED":
			# bossao_pause toggles the pause state and returns the new state
			# 1 = paused, 0 = playing
			pauseresult = bossao.bossao_pause()
			if pauseresult == 1:
				self.state = "PAUSED"
				self.sleeplength = 1
			else:
				self.state = "PLAYING"
				self.sleeplength = .1

	def handleNextCommand(self):
		self.songqueue.next()
		self.startPlaying()

	def handleJumpCommand(self, index):
		if 0 <= index < len(self.songqueue.getSongIDs()):
			self.songqueue.jump(index)
			self.startPlaying()

	def run(self):
		commandhandlers = {"play": self.handlePlayCommand,
				   "stop": self.handleStopCommand,
				   "pause": self.handlePauseCommand,
				   "next": self.handleNextCommand}
		self.aoinitialized = 0
		session = Session()
		self.configfile = session['cfg']
		self.lib = bossao.bossao_new(self.configfile, None)
		#self.mixer = bossao.bossao_new_mixer()
		self.playedflag = 0

		if self.dbh.getSongCacheSize() == 0:
			log.debug("audio", "No songs in database.  Waiting for some to get imported.")
			self.state = "EMPTY DB"
			time.sleep(.5)
			while self.dbh.getSongCacheSize() == 0 and session.hasKey('shutdown') == 0:
				time.sleep(.5)

		#bossao.bossao_open_mixer(self.mixer, self.configfile.get("mixerdevice"), self.configfile.get("mixertype"))
		self.initializeao()

		if self.shutdown == 0:
			self.songdetails = None
			if self.configfile.getBoolean("stoponstart") == True:
				log.debug("audio", "Starting stopped")
				self.state = "STOPPED"
				self.sleeplength = 1
			else:
				self.sleeplength = .1
				self.startPlaying()
			log.debug("audio", "Starting player loop")

			while self.shutdown == 0:
				try:
					self._update_status()
					try:
						currentcommand = self._pop_cmd()
						if currentcommand is not None:
							log.debug("audio", "We have a command...%s",  currentcommand)
							# call the handler for the current command
							# handlers will change self.state and self.sleeplength
							commandhandlers[currentcommand]()
					except KeyError:
						# We have to special case playindex because it has an argument
						if currentcommand[:len("playindex")] == "playindex" and len(currentcommand[len("playindex"):]) > 0:
							try:
								tmpsongid = int(currentcommand[len("playindex"):])
								self.handleJumpCommand(tmpsongid)
							except Exception:
								log.exception("Error in playindex")
						else:
							log.error("Got unknown command %s", currentcommand)

				except EndOfQueueException.EndOfQueueException:
					log.debug("audio", "No songs left to play.  Stopping...")
					self.state = "STOPPED"
					self.sleeplength = 1
				time.sleep(self.sleeplength)

			time.sleep(1)
			log.debug("audio", "Calling shutdown")
			#bossao.bossao_shutdown(self.lib)
			#bossao.bossao_stop ()
			bossao.bossao_free ()
		#bossao.bossao_close_mixer(self.mixer)
		log.debug("audio", "Closing lib")

# vim:ts=8 sw=8 noet

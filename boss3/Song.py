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

#import xinesong.xinesong

class Song:

	songint = None
	xinelib = None
	playedflag = None
	songid = None

	def __init__(self, xinelib):
		self.playedflag = 0
		self.songid = 0
		self.songint = xinesong.xinesong.song()
		self.xinelib = xinelib

	def songInit(self):
		xinesong.xinesong.song_init(self.songint, self.xinelib)

	def songOpen(self):
		xinesong.xinesong.song_open(self.songint)

	def songClose(self):
		xinesong.xinesong.song_close(self.songint)

	def songStart(self):
		xinesong.xinesong.song_start(self.songint)

	def getMetaData(self):
		return xinesong.xinesong.getMetaData(self.songint)
	
	def getVolume(self):
		return xinesong.xinesong.getVolume(self.songint)
	
	def setVolume(self,volume):
		xinesong.xinesong.setVolume(self.songint,volume)

	def play(self):
		xinesong.xinesong.song_play(self.songint)

	def pause(self):
		xinesong.xinesong.song_pause(self.songint)
	
	def stop(self):
		xinesong.xinesong.song_stop(self.songint)

	def start(self):
		xinesong.xinesong.song_start(self.songint)

	def finish(self):
		xinesong.xinesong.song_finish(self.songint)
	
	def restart(self):
		xinesong.xinesong.self_restart(self.songint)

	def updateInfo(self):
		xinesong.xinesong.song_updateinfo(self.songint)
	

# vim:ts=8 sw=8 noet

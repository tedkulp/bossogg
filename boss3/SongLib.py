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

import Song
import xinesong.xinesong

class SongLib:

	thelib = None
	filename = ""

	def __init__(self):
		self.thelib = xinesong.xinesong.xinelib()

	def initLib(self, audiodriver=None):
		if audiodriver is None:
			audiodriver = "auto"
		return xinesong.xinesong.initlib(self.thelib, audiodriver)

	def closeLib(self):
		xinesong.xinesong.closelib(self.thelib)
		
	def createSong(self):
		somesong = Song.Song(self.thelib)
		return somesong

# vim:ts=8 sw=8 noet

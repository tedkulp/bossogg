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

class Control:

	"""
	Control realm.  This handles all commands that control playback in some
	way or form.  Basically, if it's on the front of a CD Player, it's here.


	control("pause"): Puts the player into a paused state if it's playing.
	If already paused, it does nothing.

	Parameters:
	* None

	Returns:
	* int(always 1)


	control("play"): Puts the player into a playing state if it's paused
	or stopped.  If already playing, it does nothing.

	Parameters: 
	* None

	Returns: 
	* int(always 1)
	

	control("stop"): Put the player into a stopped state.  If alrady
	stopped, it does nothing.

	Parameters:
	* None

	Returns:
	* int(always 1)
	

	control("next"): Immediately starts playing the next song.  If
	there are no songs left in the queue, it will use the endofqueue
	parameter to choose what to do next.  

	Parameters:
	* None

	Returns:
	* int(always 1)
	"""

	def handleRequest(self, cmd, argstuple):

		session = Session()

		args = []
		for i in argstuple:
			args.append(i)

		if (session.hasKey('cmdint')):
			cmdint = session['cmdint']

			if (cmd == "pause"):
				#cmdint.push_cmd("pause")
				cmdint.control.pause()
				return True
			elif (cmd == "stop"):
				#cmdint.push_cmd("stop")
				cmdint.control.stop()
				return True
			elif (cmd == "play"):
				#cmdint.push_cmd("play")
				cmdint.control.play()
				return True
			elif (cmd == "next"):
				#cmdint.push_cmd("play")
				cmdint.control.next()
				return True
			elif (cmd == "playindex"):
				if len(args) == 1:
					cmdint.control.playIndex(args[0])
					return True
				else:
					return xmlrpclib.Fault(2, "Error in given parameters")

# vim:ts=8 sw=8 noet

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

class Info:
	"""
	Info realm.  These commands query data from the
	running server and are not reflected by database
	locks.  These are generally safe commands which can
	be used by any security level.


	info("status"): Return the status of the server at
	that point in time.  Basic display information is
	shown, including that the player is doing, what songid
	is playing, the queue status and other pertinent
	information a client needs so that it knows what to do
	next.

	Parameters:
	* None

	Returns:
	* Struct
	  * queuemd5sum - string
	  * status - string
	  * queueindex - int (or string as "endofqueue")
	  * songid - int
	  * playedpercentage - float
	  * timeplayed - float
	  * songlength - float


	info("endofqueue"): Returns the currently selected
	endofqueue action.

	Parameters:
	* None

	Returns:
	* string
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

			if (cmd == "status"):
				return cmdint.info.status()
			elif (cmd == "endofqueue"):
				return cmdint.info.endofqueue()

# vim:ts=8 sw=8 noet

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

class Util:
	"""
	Util realm.  This will contain basic functions that don't
	really fit anywhere else.  They can generally have a very low
	security level.


	util("version"): Returns version information about the running
	server.

	Parameters:
	* None

	Returns:
	* Struct
	  * version - string
	  * name - string
	"""

	def handleRequest(self, cmd, argstuple):

		session = Session()

		args = []
		for i in argstuple:
			args.append(i)

		if (session.hasKey('cmdint')):
			cmdint = session['cmdint']

			if cmd == "version":
				return cmdint.util.version()

# vim:ts=8 sw=8 noet

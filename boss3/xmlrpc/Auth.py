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

class Auth:
	"""
	Auth realm.  This will handle all aspects of logging in and
	out of the server.

	auth("login"): Establish a login with the server.  This will
	check security and generate a sessionid for the client to use
	when running all other commnads.  This sessionid will time out
	after 30 mintues if not used.  Will return a fault if the proper
	amount of parameters are not given.  Will return a string with
	the value "Permissions Denied" if the username/password
	combination is not correct.

	Parameters:
	* username - string
	* password - string

	Returns:
	* sessionid - string
	"""

	def handleRequest(self, cmd, argstuple):

		session = Session()

		args = []
		for i in argstuple:
			args.append(i)

		if (session.hasKey('cmdint')):
			cmdint = session['cmdint']

			if (cmd == "login"):
				#TODO: Handle the callback
				if (len(args)>1):
					tmp = cmdint.auth.login(args[0], args[1])
					if (tmp != None):
						return tmp
					else:
						return "Permission Denied"
				else:
					return xmlrpclib.Fault(1, "Not enough parameters given")
			elif (cmd == "logout"):
				if (len(args)==1):
					return cmdint.auth.logout(args[0])
                        else:
				return xmlrpclib.Fault(1, "Invalid command in the auth realm")

# vim:ts=8 sw=8 noet

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

import boss3.xmlrpc.bossxmlrpclib as xmlrpclib
import types
import bosslog

log = bosslog.getLogger()
	
def decode(utfbin):
	log.debug("utf", "UTFstring decode()")
	if not isinstance(utfbin, types.StringType) and hasattr(utfbin, "data"):
		if utfbin.data == '__NULL__':
			log.debug("utf", "returning: ''")
			return ''
		else:
			log.debug("utf", "returning: '%s'", utfbin.data)
			return utfbin.data
	else:
		#if isinstance(utfbin,types.StringType):
		return utfbin
		#else:
		#  return None

def encode(string):
	log.debug("utf", "UTFstring encode(%s:%s)", type(string), string)
	if isinstance(string, types.StringType):
		log.debug("utf", "its a string")
		if string == '':
			string='__NULL__'
		ustring=unicode(string,'latin-1')
	else:
		if isinstance(string, types.UnicodeType):
			ustring=string
		else:
			log.debug("utf", "Not a string... it's a %s", type(string))
			#if isinstance(string,xmlrpclib.Binary):
			return string
			#else:
			#   return None
	log.debug("utf", "ustring: %s", type(ustring))
	tmp=xmlrpclib.Binary(ustring.encode('latin-1'))
	log.debug("utf", "encoded: %s %s", type(tmp.data),tmp.data)
	return tmp

# vim:ts=8 sw=8 noet

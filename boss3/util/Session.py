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

class Session:
	idlist = {}
	_shared_state = {}
	def __init__(self):
		self.__dict__=self._shared_state
	def __getitem__(self,key):
		if key in self.idlist:
			return self.idlist[key]
		else:
			return None
	def __setitem__(self,key,value):
		self.idlist[key]=value
	def __delitem__(self,key):
		self.idlist[key]=None
	def hasKey(self,key):
		return self.idlist.has_key(key)
	#To use in a module
	#from Session import *
	#session=Session()
	#Set up a new id item as Class Config
	#from Config import *	
	#session['idname']=Config()
	#Access Class Config variable example
	#session['idname'].example='some example text'


# vim:ts=8 sw=8 noet

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

class Util:

	def getPermissionsFromInt(self,permnum):
		result = {}
		result['list'] = (permnum & 1 == 1)
		result['info'] = (permnum & 2 == 2)
		result['auth'] = (permnum & 4 == 4)
		result['control'] = (permnum & 8 == 8)
		result['db'] = (permnum & 16 == 16)
		result['load'] = (permnum & 32 == 32)
		result['edit'] = (permnum & 64 == 64)
		result['set'] = (permnum & 128 == 128)
		result['util'] = (permnum & 256 == 256)
		return result

	def getIntFromPermissions(self, perms):
		result = 0

		if perms['list']:
			result += 1
		if perms['info']:
			result += 2
		if perms['auth']:
			result += 4
		if perms['control']:
			result += 8
		if perms['db']:
			result += 16
		if perms['load']:
			result += 32
		if perms['edit']:
			result += 64
		if perms['set']:
			result += 128
		if perms['util']:
			result += 256

		return result

if __name__ == "__main__":
	blah = Util()
	print blah.getIntFromPermissions(blah.getPermissionsFromInt(10))
	print blah.getIntFromPermissions(blah.getPermissionsFromInt(200))
	print blah.getIntFromPermissions(blah.getPermissionsFromInt(0))
	print blah.getIntFromPermissions(blah.getPermissionsFromInt(312))

# vim:ts=8 sw=8 noet

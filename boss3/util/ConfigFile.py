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

import ConfigParser
import os
import os.path
import sys
from boss3.util.Session import *
from boss3.util import Logger

class ConfigFile:

	defaultfile = """#Boss3 Configuration File
#Configuration options for the Boss Ogg Music Server.

[DEFAULT]

#Loglevel will set how much information is sent to the terminal from the server.
#1 is a normal level that just gives some basic startup and shutdown
#information.  2 is for debugging purposes.  3 will basically flood the term
#with more information than anyone could possibly use.  0 will surpress
#everything.
loglevel: %(default_loglevel)s

#Startpaused will put the server into a stopped state after startup.  This is
#very useful for daemons because you don't have to worry about the server blasting
#music when you're not excepting it. Valid entries are: 1/0, True/False, Yes/No
stoponstart: %(default_stoponstart)s

#Detach the server from the tty when started.  This works well for using boss
#as a daemon that starts up when the machie boots.  All output will be
#supressed with this option (Equivilent to loglevel=0).  Valid entries are: 1/0,
#True/False, Yes/No
detach: %(default_detach)s

#End of queue option.  This basically says what to do when end of the queue is
#reached or is empty.  This can be changed on the fly, but the default will be
#used whenever the server is started up.  Valid options are:
#allstraight - This will run straight through all of the songs in the db.  It
#              will remember where it left off in cause the queue is temporarily
#              filled.
#allshuffle  - This will randomly pick through all of the songs in the database
#              using a weighted algorithm.
#repeat      - Go right to the beginning of the queue and start playing it again.
#stop        - Stop after the last song and wait for something else to happen.  If
#              you queue a new song while stopped, the new song will be the first
#              one to play.
endofqueue: %(default_endofqueue)s

#End of queue parameter.  Some end of queue types take an extra parameter.  This
#goes here.
endofqueueparam: %(default_endofqueueparam)s

#Audio drvier selection.  If left on auto, it should automatically figure out
#which driver to use.  Can also be overridden to force a particular driver to
#be used.  Available drivers are system-dependant and the --list-audio-plugsins
#command line switch in bossogg will show you a list of available plugins.
audiodriver: %(default_audiodriver)s

#Mixer type.  This should conincide with the audio driver above, but, auto does
#not work.   Can either be "oss" or "alsa".
mixertype: %(default_mixertype)s

#Mixer device.  Where in the file system the filesystem the mixer resides.  For
#oss, it will usually be "/dev/mixer".  For alsa, it's generally "default".  Only
#change if volume changes aren't getting recognized.
mixerdevice: %(default_mixerdevice)s

[SHOUT]

#Set to 1 to enable icecasting
enable: %(shout_enable)s

#Host of the icecast2 server
host: %(shout_host)s

#Icecast2 user to log in as
user: %(shout_user)s

#Password for the user
password: %(shout_password)s

#Mount of the stream
mount: %(shout_mount)s

#Port number to connect to
port: %(shout_port)s

#Bitrate to reencode audio at
bitrate: %(shout_bitrate)s

#Title to user for metadata
title: %(shout_title)s

[RIPPER]

#Number of CD sectors to read in each pass
read_sectors: %(ripper_read_sectors)s

#Bitrate to rip the ogg files at.
bitrate: %(ripper_bitrate)s

#Never go below this bitrate (NOTE: must also use managed!)
min_bitrate: %(ripper_min_bitrate)s

#Never go above this bitrate (NOTE: must also use managed!)
max_bitrate: %(ripper_max_bitrate)s

#Use the bitrate manager
managed: %(ripper_managed)s

#Ogg quality level. 1.0 is the highest.
quality: %(ripper_quality)s

#Use VBR
quality_set: %(ripper_quality_set)s

#CD device to rip from
device: %(ripper_device)s

[CONF_FILE]
#Don't touch this
version: %(conf_file_version)s

"""

	version = 3
	cfgparser = None
	filename = ""

	def createFile(self):
		return self.defaultfile % {
			"default_loglevel" : self.cfgparser.get("DEFAULT", "loglevel"),
			"default_stoponstart" : self.cfgparser.get("DEFAULT", "stoponstart"),
			"default_detach" : self.cfgparser.get("DEFAULT", "detach"),
			"default_endofqueue" : self.cfgparser.get("DEFAULT", "endofqueue"),
			"default_endofqueueparam" : self.cfgparser.get("DEFAULT", "endofqueueparam"),
			"default_audiodriver" : self.cfgparser.get("DEFAULT", "audiodriver"),
			"default_mixertype" : self.cfgparser.get("DEFAULT", "mixertype"),
			"default_mixerdevice" : self.cfgparser.get("DEFAULT", "mixerdevice"),

			"shout_enable" : self.cfgparser.get("SHOUT", "enable"),
			"shout_host" : self.cfgparser.get("SHOUT", "host"),
			"shout_user" : self.cfgparser.get("SHOUT", "user"),
			"shout_password" : self.cfgparser.get("SHOUT", "password"),
			"shout_mount" : self.cfgparser.get("SHOUT", "mount"),
			"shout_port" : self.cfgparser.get("SHOUT", "port"),
			"shout_bitrate" : self.cfgparser.get("SHOUT", "bitrate"),
			"shout_title" : self.cfgparser.get("SHOUT", "title"),

			"ripper_read_sectors" : self.cfgparser.get("RIPPER", "read_sectors"),
			"ripper_bitrate" : self.cfgparser.get("RIPPER", "bitrate"),
			"ripper_min_bitrate" : self.cfgparser.get("RIPPER", "min_bitrate"),
			"ripper_max_bitrate" : self.cfgparser.get("RIPPER", "max_bitrate"),
			"ripper_managed" : self.cfgparser.get("RIPPER", "managed"),
			"ripper_quality" : self.cfgparser.get("RIPPER", "quailty"),
			"ripper_quality_set" : self.cfgparser.get("RIPPER", "quality_set"),
			"ripper_device" : self.cfgparser.get("RIPPER", "device"),

			"conf_file_version" : self.cfgparser.get("CONF_FILE","version")
		}

	def setDefaults(self):
		self.cfgparser.set("DEFAULT","loglevel", "1")
		self.cfgparser.set("DEFAULT","stoponstart", "No")
		self.cfgparser.set("DEFAULT","detach", "No")
		self.cfgparser.set("DEFAULT","endofqueue", "allstraight")
		self.cfgparser.set("DEFAULT","endofqueueparam", "")
		self.cfgparser.set("DEFAULT","audiodriver", "auto")
		self.cfgparser.set("DEFAULT","mixertype","oss"),
		self.cfgparser.set("DEFAULT","mixerdevice","/dev/mixer"),

		self.cfgparser.set("SHOUT", "enable", "0")
		self.cfgparser.set("SHOUT", "host", "127.0.0.1")
		self.cfgparser.set("SHOUT", "user", "user")
		self.cfgparser.set("SHOUT", "password", "password")
		self.cfgparser.set("SHOUT", "mount", "/bossogg.ogg")
		self.cfgparser.set("SHOUT", "port", "8000")
		self.cfgparser.set("SHOUT", "bitrate", "192")
		self.cfgparser.set("SHOUT", "title", "Boss Ogg Streaming Server")
		
		self.cfgparser.set("RIPPER","read_sectors", "64")
		self.cfgparser.set("RIPPER","bitrate", "196")
		self.cfgparser.set("RIPPER","min_bitrate", "0")
		self.cfgparser.set("RIPPER","max_bitrate", "0")
		self.cfgparser.set("RIPPER","managed", "0")
		self.cfgparser.set("RIPPER","quailty", "1.0")
		self.cfgparser.set("RIPPER","quality_set", "0")
		self.cfgparser.set("RIPPER","device", "auto")

		self.cfgparser.set("CONF_FILE","version",str(self.version))

	def __init__(self,filename=None):
	
		session = Session()

		self.cfgparser = ConfigParser.ConfigParser()
		self.cfgparser.add_section("RIPPER")
		self.cfgparser.add_section("SHOUT")
		self.cfgparser.add_section("CONF_FILE")

		if (filename != None):
			self.filename = filename
		elif (os.getenv("BOSS_CONF") != None):
			self.filename = os.getenv("BOSS_CONF")
		else:
			#if (os.getlogin() == "root"):
			if session['userdir']:
				self.filename = session['userdir'] + "/.bossogg/bossogg.conf"
			elif os.geteuid() == 0:
				prefix = "/etc"
				if (sys.prefix != "/usr"):
					prefix = sys.prefix + "/etc"
				self.filename = prefix + "/bossogg.conf"
			else:
				self.filename = os.path.expanduser("~/.bossogg/bossogg.conf")

		Logger.log("Using config file:" + self.filename)
		if (os.path.exists(self.filename) == True):
			Logger.log("File Exists... opening")
			self.load()
			try:
				blah = self.get("version","CONF_FILE")
				if blah < self.version:
					self.set("version", "CONF_FILE", str(self.version))
					self.save()
			except Exception:
				Logger.log("Conf file is too old.  Removing and making a new one.",1)
				self.setDefaults()
				self.save()
		else:
			self.setDefaults()
			Logger.log("File Does Not Exist... creating")
			head, tail = os.path.split(self.filename)
			if (os.path.exists(head) == False):
				os.makedirs(head)
			self.save()

	def printSettings(self):
		print str(self.cfgparser.options())

	def load(self):
		self.setDefaults()
		self.cfgparser.read(self.filename)

	def save(self):
		tmpfile = file(self.filename, 'w')
		#tmpfile.write(self.defaultfile)
		tmpfile.write(self.createFile())
		tmpfile.close()

	def get(self,setting,section="DEFAULT"):
		tmp = self.cfgparser.get(section, setting)
		return tmp 
	def getBoolean(self,setting,section="DEFAULT"):
		tmp = self.cfgparser.getboolean(section, setting)
		return tmp 

	def set(self,setting,value,section="DEFAULT"):
		self.cfgparser.set(section, setting, value)

# vim:ts=8 sw=8 noet

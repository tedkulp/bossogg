#
# Bossogg logger
#


# Instructions for use:

# The way the new logging system works is you import this module and then get the log
# instance that is common to the whole program.

# from boss3.util import bosslog

# log = bosslog.getLogger()

# There are many different log levels:
# 	* exception
# 	* critical
# 	* error
# 	* warning
# 	* info
# 	* debug

# they are all called the same way log.<level>(<fmtstr>, arg, ...) with the exception
# debug which is log.debug(<channel>, <fmtstr>, arg, ...). The channel on debug refers to
# the debug channel. This allows you to turn channels on/off when you are debugging, so for
# example you can only choose to see the sqlquery's going to the DB.
# *NOTE* the arguments are seperated by commas and are mot %'ed into the fmtstr. This saves us
# time in no having to render the string if we don't display the message. When your dealing with
# a list that has 2000+ dictionaries in it, this saves *alot* of time.

# When you call log.exception() it will log the message you pass to the function but it will also
# log the current backtrace to the log too. This means if you call it in a try: except: you will
# can log exceptions.

# Thats about it. Check out some of the files. Database.py probably is the biggest user at this
# stage. To enable debug output use -D option on the commands.

import logging, string, sys, os
from logging import INFO,DEBUG,WARNING
from types import *

if string.lower(__file__[-4:]) in ['.pyc', '.pyo']:
    _srcfile = __file__[:-4] + '.py'
else:
    _srcfile = __file__
_srcfile = os.path.normcase(_srcfile)

_srcfiles = (_srcfile, logging._srcfile)

channels = (
	"import", "database", "interact", "file",
	"misc", "network", "audio", "config",
	"sql", "sqlquery", "sqlresult", "cache", "funcs", "lock",
	"queue", "xmlrpc", "oldlog", "utf", "bossrpc"
	)

#
# a_c = { <file> : [ active_channels ], }
# file = * for default.
# 
active_channels = { "*":[] }

class Filter(logging.Filter):
	def filter(self, record):
		# If it's not the debug level log it
		if record.levelno != logging.DEBUG:
			return True
		# Check if a channel is set. If not log it. (should be a channel!)
		msg = record.msg
		if msg[0] != "|":
			return True
		# Extract Channel msg = "|channel| ...."
		channel = msg[1:msg[1:].index("|")+1]
		file = record.filename
		if file in active_channels:
			if channel in active_channels[file]:
				return True
		else:
			if channel in active_channels["*"]:
				return True
			
		return False
		

class Logger(logging.Logger):
	def __init__(self, name):
		logging.Logger.__init__(self, name)
		self.stdout = logging.StreamHandler()
		self.format = logging.Formatter("[Tx%(thread)d:%(levelname)s:%(module)s:%(lineno)d] %(message)s")
		self.stdout.setFormatter(self.format)
		self.addHandler(self.stdout)
		self.addFilter(Filter())

	def logtofile(self, filename):
		self.file = logging.FileHandler(filename)
		self.file.setFormatter(self.format)
		self.addHandler(self.file)

	def findCaller(self, stack=0):
		"""
		Find the stack frame of the caller so that we can note the source
		file name and line number.
		"""
		f = sys._getframe(1)
		
		while 1:
			co = f.f_code
			filename = os.path.normcase(co.co_filename)
			if filename in _srcfiles:
				f = f.f_back
				continue
			for st in range(0, stack):
				f = f.f_back
			co = f.f_code
			filename = os.path.normcase(co.co_filename)
			return filename, f.f_lineno

	def debug(self, channel, msg, *args, **kwargs):
		if self.manager.disable >= DEBUG:
			return
		if DEBUG >= self.getEffectiveLevel():
			try:
				msg % args
				apply(self._log, (DEBUG, "|"+channel+"| "+msg, args), kwargs)
			except:
				self.exception("Format error in string")

	def _log(self, level, msg, args, exc_info=None, stack=1):
		if _srcfile:
			fn, lno = self.findCaller(stack=stack)
		else:
			fn, lno = "<unknown file>", 0
		if exc_info:
			exc_info = sys.exc_info()
		record = self.makeRecord(self.name, level, fn, lno, msg, args, exc_info)
		self.handle(record)


logging.setLoggerClass(Logger)

def getLogger():
	return logging.getLogger("bossogg")

log = getLogger()

def help():
	print "Debug usage:"
	print "-D [+|-]<channel>[:file],[+|-]<channel>[:file],..."
	print
	print "You can create the amount of logging you want by using the following channels"
	chans = list(channels)
	chans.sort()
	num=5
	for i in range(0, len(channels)/num):
		print "\t"+", ".join(chans[i*num:i*num+num])
	print
	print "You can also use channel 'all' to log all channels. Prefixing a channel with + or -"
	print "will enable/disable that channel. EG: all,-sqlresult,-all:UTFstring.py will show everything"
	print "but sql reuslts, and anything from the file UTFstring.py"
	print

	sys.exit(0)
	

def set_channels(chans):
	global channels, active_channels

	active_channels["*"] = []
	
	if type(chans) is StringType:
		chans = map(lambda x: x.strip(), chans.split(","))
		
	for channel in chans:
		if channel == "help":
			help()

		if channel.find(":") != -1:
			channel, file = channel.split(":")[:2]
		else:
			file = None

		if channel[0] in ("+", "-"):
			switch = channel[0]
			channel = channel[1:]
		else:
			switch = None

		if file and file not in active_channels and "*" in active_channels:
			active_channels[file] = []
			active_channels[file].extend(active_channels["*"])

		if channel.lower() == "all":
			channel = channels
		else:
			channel = [channel, ]
				
		if file:
			if switch == "-":
				map(lambda c: c in active_channels[file] and active_channels[file].remove(c), channel)
			else:
				map(lambda c: c not in active_channels[file] and active_channels[file].append(c), channel)
		else:
			for f in active_channels.keys():
				if switch == "-":
					map(lambda c: c in active_channels[f] and active_channels[f].remove(c), channel)
				else:
					map(lambda c: c not in active_channels[f] and active_channels[f].append(c), channel)
		

	log.info("Debug channels active: %s", repr(active_channels))


def channel_list():
	return  channels

def get_channels():
	return active_channels


#!/usr/bin/python

import ripper
from util.Session import *
import util.ConfigFile


filenames = ["1.ogg", "2.ogg", "3.ogg", "4.ogg", "5.ogg", "6.ogg", "7.ogg", "8.ogg", "9.ogg", "10.ogg"]
device = "/dev/cdrom"
session = Session()
session['loglevel'] = 1
cfgparser = util.ConfigFile.ConfigFile()
tags = None
#tags = ripper.getCDDB(device)
#print tags
#if you want, pass a list of filenames. or, pass an int and generate one based  
#on the track number and songname, eg. "04 - This is a sample.ogg"
ripper.pyrip(device, cfgparser, tags, 0)


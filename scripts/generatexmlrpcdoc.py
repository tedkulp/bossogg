#!/usr/bin/env python

import datetime
from boss3 import version
from boss3.xmlrpc import Load
from boss3.xmlrpc import Info
from boss3.xmlrpc import List
from boss3.xmlrpc import Set
from boss3.xmlrpc import Control
from boss3.xmlrpc import DB
from boss3.xmlrpc import Auth
from boss3.xmlrpc import Edit
from boss3.xmlrpc import Util

fileobj = file("xmlrpcdoc.txt", "w")

fileobj.write("Auto-generated xmlrpc docs for: %s %s\n" % (version._boss_name, version._boss_version))
fileobj.write("Generated on: %s\n\n" % datetime.datetime.now())

fileobj.write("--= Info =--\n")
fileobj.write(Info.Info.__doc__ + "\n")

fileobj.write("--= Control =--\n")
fileobj.write(Control.Control.__doc__ + "\n")

fileobj.write("--= List =--\n")
fileobj.write(List.List.__doc__ + "\n")

fileobj.write("--= Load =--\n")
fileobj.write(Load.Load.__doc__ + "\n")

fileobj.write("--= Set =--\n")
fileobj.write(Set.Set.__doc__ + "\n")

fileobj.write("--= DB =--\n")
fileobj.write(DB.DB.__doc__ + "\n")

fileobj.write("--= Auth =--\n")
fileobj.write(Auth.Auth.__doc__ + "\n")

fileobj.write("--= Edit =--\n")
fileobj.write(Edit.Edit.__doc__ + "\n")

fileobj.write("--= Util =--\n")
fileobj.write(Util.Util.__doc__ + "\n")

fileobj.close()

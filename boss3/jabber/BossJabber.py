import jabber.jabber
import util.Logger
import sys
import codecs
import thread
from util.Session import *

class BossClientInterface:

	jabbercli = None
	threadid = None

	def start_server(self):
		self.jabbercli = BossJabberInterface()

	def __init__(self):
		#Register the lib with the CommandInterface here
		self.threadid = thread.start_new_thread(self.start_server, ())

class BossJabberInterface:

	con = None
	session = None

	def messageCB(self,con,msg):
		util.Logger.log("message from " + str(msg.getFrom()) + ":" + str(msg.getBody()))
		self.handleCommand(msg.getBody())

	def presenceCB(self,con,prs):
		print "presence:" + str(con) + " " + str(prs)

	def iqCB(self,con,iq):
		print "iq:" + str(con) + " " + str(iq)

	def disconnectedCB(self,con):
		print "disconnected:" + str(con)

	def handleCommand(self,cmd):
		cmdint = self.session['cmdint']
		if (cmd == "pause"):
			cmdint.doPause()
		elif (cmd == "play"):
			cmdint.doPlay()
		elif (cmd == "stop"):
			cmdint.doStop()

	def __init__(self):
		#Setup the connection to the jabber server
		#self.con = jabber.jabber.Client(host="jabber.wishy.org", debug=True, log=sys.stderr)
		self.session = Session()
		self.con = jabber.jabber.Client(host="jabber.wishy.org")
		try:
			self.con.connect()
		except IOError, e:
			util.Logger.log("Couldn't connect: %s" % e)
		else:
			util.Logger.log("Connected to jabber server")

		self.con.setMessageHandler(self.messageCB)
		self.con.setPresenceHandler(self.presenceCB)
		self.con.setIqHandler(self.iqCB)
		self.con.setDisconnectHandler(self.disconnectedCB)

		if self.con.auth("boss3", "boss3", "boss3"):
			util.Logger.log("Logged into jabber server")
		else:
			util.Logger.log("Couldn't log in... " + self.con.lastErr + self.con.lastErrCode)

		self.con.sendInitPresence()
		while(self.session.hasKey('shutdown') == 0):
			self.con.process(1)

# vim:ts=8 sw=8 noet

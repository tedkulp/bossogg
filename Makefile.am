SUBDIRS = boss3
bin_SCRIPTS = bossogg bosync boshell
EXTRA_DIST = bossogg bosync boshell
nobase_python_PYTHON=boss3/CommandInterface.py boss3/Database.py boss3/DatabaseSchema.py boss3/Player.py boss3/UserManager.py boss3/version.py boss3/__init__.py boss3/bocmd.py boss3/bossao/__init__.py boss3/bossao/bossao.py boss3/metadata/__init__.py boss3/metadata/id3.py boss3/util/__init__.py boss3/util/ConfigFile.py boss3/util/Logger.py boss3/util/Queue.py boss3/util/Session.py boss3/util/UTFstring.py boss3/util/Util.py boss3/xmlrpc/__init__.py boss3/xmlrpc/Auth.py boss3/xmlrpc/Boss2XmlRpcServer.py boss3/xmlrpc/Control.py boss3/xmlrpc/DB.py boss3/xmlrpc/Edit.py boss3/xmlrpc/Info.py boss3/xmlrpc/List.py boss3/xmlrpc/Load.py boss3/xmlrpc/Set.py boss3/xmlrpc/SimpleXMLRPCServer.py boss3/xmlrpc/Util.py boss3/xmlrpc/XmlRpcServer.py boss3/xmlrpc/xmlrpclib.py boss3/bossexceptions/__init__.py boss3/bossexceptions/EndOfQueueException.py boss3/xmlrpc/bossxmlrpclib.py boss3/xmlrpc/bossxmlrpcserver.py boss3/util/bosslog.py boss3/util/logging/__init__.py boss3/util/logging/config.py boss3/util/logging/handlers.py

install-exec-local:
	mkdir -p ${pythondir}/boss3
	mkdir -p ${pythondir}/boss3/bossao
	mkdir -p ${pythondir}/boss3/bossexceptions
	mkdir -p ${pythondir}/boss3/metadata
	mkdir -p ${pythondir}/boss3/util
	mkdir -p ${pythondir}/boss3/xmlrpc

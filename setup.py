#!/usr/bin/env python

from distutils.core import setup, Extension
import distutils.sysconfig
from distutils.command.build import build
import os,sys,string
from bossogg import version

def findHeader(headerfile, firstlevel, secondlevel=None):
	for i in firstlevel:
		if secondlevel is not None:
			for j in secondlevel:
				pathtosearch = i + j
				if os.path.exists(pathtosearch + "/" + headerfile):
					print "Checking for %s...   [found: %s]" % (headerfile, pathtosearch)
					return pathtosearch
		else:
			pathtosearch = i
			if os.path.exists(pathtosearch + "/" + headerfile):
				print "Checking for %s...   [found: %s]" % (headerfile, pathtosearch)
				return pathtosearch
	print "Checking for %s...   [failed]" % pathtosearch
	return None

def findLib(headerfile, firstlevel, secondlevel=None):
	for i in firstlevel:
		if secondlevel is not None:
			for j in secondlevel:
				pathtosearch = i + j
				if os.path.exists(pathtosearch + "/lib" + headerfile + ".so"):
					print "Checking for %s library...   [found: %s]" % (headerfile, pathtosearch)
					return pathtosearch
		else:
			pathtosearch = i
			if os.path.exists(pathtosearch + "/lib" + headerfile + ".so"):
				print "Checking for %s library...   [found: %s]" % (headerfile, pathtosearch)
				return pathtosearch
	print "Checking for %s library...   [failed]" % pathtosearch
	return None

if "help" not in sys.argv:
	print "\n===Running Bossogg Setup===\n"

	if os.path.exists("metadata/id3_wrap.c") == False or os.path.exists("xinesong/xinesong_wrap.c") == False or os.path.exists("ripper/ripper_wrap.c") == False:
		print "Swig interfaces not found...  generating them...\n"
		os.system("sh swig.sh")

	doogg = True
	domp3 = True
	doflac = True
	doripper = True
	if "--disable-ogg" in sys.argv:
		doogg = False
	if "--disable-mp3" in sys.argv:
		domp3 = False
	if "--disable-flac" in sys.argv:
		doflac = False
	if "--disable-ripper" in sys.argv:
		doripper = False

	#Setup some default values
	packages_list=[]
	ext_modules_list=[]

	metadata_libs=[]

	#Setup local packages
	packages_list.append('bossogg')
	packages_list.append('bossogg.util')
	packages_list.append('bossogg.jabber')
	packages_list.append('bossogg.xmlrpc')
	packages_list.append('bossogg.bossexceptions')


	#Look for xine libs
	#print "[Checking for xine headers]"
	aofound = True
	print "\n[Checking for libao headers]"
	ao1 = findHeader("ao.h", ["/usr/include", "/usr/local/include"], ["/ao",""])
	print "\n[Checking for libao libraries]"
	ao2 = findLib("ao", ["/usr/lib", "/usr/local/lib"], ["/ao",""])
	if ao1 is None or ao2 is None:
		print "libao headers and/or libs not found.  Libao must be installed to use this program.  Exiting..."
		aofound = False
		sys.exit(-1)
	#xine_cflags = []
	#xine_cflags.append("-I./xinesong")
	#xine_cflags_file = os.popen("xine-config --cflags")
	#tmp = xine_cflags_file.readline()
	#if string.find(tmp,"-I") == -1:
	#	xinefound = False
	#	print "Xine cflags Not Found!!!"
	#	sys.exit(-1)
	#else:
	#	print "Xine cflags Found: %s" % string.rstrip(tmp)
	#tmp2 = string.split(tmp)
	#for i in tmp2:
	#	xine_cflags.append(i)
	#
	#xine_libs = []
	#xine_libs_file = os.popen("xine-config --libs")
	#tmp = xine_libs_file.readline()
	#if string.find(tmp,"-lxine") == -1:
	#	xinefound = False
	#	print "Xine libs Not Found!!!"
	#	sys.exit(-1)
	#else:
	#	print "Xine libs Found: %s" % string.split(tmp)
	#tmp2 = string.split(tmp)
	#for i in tmp2:
	#	xine_libs.append(i)

	if aofound == True:
		#ext_modules_list.append(Extension("bossogg.xinesong._xinesong",["bossogg/xinesong/xinesong.c","bossogg/xinesong/xinesong_wrap.c"],extra_compile_args=xine_cflags,extra_link_args=xine_libs))
		#packages_list.append("bossogg.xinesong")

		#Look for metadata libs
		bossao_cfiles = ["bossogg/bossao/bossao.c","bossogg/bossao/bossao_wrap.c","bossogg/bossao/oss_mixer.c"]
		metadata_cflags = []
		metadata_cflags.append("-I./bossogg/metadata")
		metadata_cflags.append("-I" + ao1)
		metadata_libs = []
		metadata_libs.append("ao")
		metadata_libs_runtime = []
		metadata_libs_extra = []
		haveogg = False
		haveflac = False
		havemp3 = False

		if doogg == True:
			print "\n[Checking for vorbis headers]"
			vorbis1 = findHeader("ogg.h", ["/usr/include", "/usr/local/include"], ["/ogg",""])
			vorbis2 = findHeader("codec.h", ["/usr/include", "/usr/local/include"], ["/vorbis",""])
			vorbis3 = findHeader("vorbisfile.h", ["/usr/include", "/usr/local/include"], ["/vorbis",""])
			vorbis7 = findHeader("vorbisenc.h", ["/usr/include", "/usr/local/include"], ["/vorbis",""])

			print "\n[Checking for vorbis libraries]"
			vorbis4 = findLib("ogg", ["/usr/lib", "/usr/local/lib"], ["/ogg",""])
			vorbis5 = findLib("vorbis", ["/usr/lib", "/usr/local/lib"], ["/vorbis",""])
			vorbis6 = findLib("vorbisfile", ["/usr/lib", "/usr/local/lib"], ["/vorbis",""])
			vorbis8 = findLib("vorbisenc", ["/usr/lib", "/usr/local/lib"], ["/vorbis",""])
			if vorbis8 is None or vorbis7 is None:
				print "vorbisenc headers and/or libs not found.  Disabling ripping support"
				doripper = False

			if vorbis1 is None or vorbis2 is None or vorbis3 is None:
				print "Headers not found.  Disabling vorbis and flac metadata support."
			else:
				haveogg = True
				bossao_cfiles.append("bossogg/bossao/ogg.c")
				metadata_cflags.append('-I' + vorbis1)
				metadata_cflags.append('-I' + vorbis2)
				metadata_cflags.append('-I' + vorbis3)
				if vorbis7 is not None:
					metadata_cflags.append('-I' + vorbis7)

				metadata_cflags.append('-DHAVE_VORBIS')

				if '-L' + vorbis4 not in metadata_libs_extra:
					metadata_libs_extra.append('-L' + vorbis4)
				if '-L' + vorbis5 not in metadata_libs_extra:
					metadata_libs_extra.append('-L' + vorbis5)
				if '-L' + vorbis6 not in metadata_libs_extra:
					metadata_libs_extra.append('-L' + vorbis6)
				if vorbis8 is not None and '-L' + vorbis6 not in metadata_libs_extra:
					metadata_libs_extra.append('-L' + vorbis8)

				metadata_libs.append('ogg')
				metadata_libs.append('vorbis')
				metadata_libs.append('vorbisfile')
				if vorbis8 is not None:
					metadata_libs.append('vorbisenc')

				if doflac == True:
					print "\n[Checking for flac headers]"
					flac1 = findHeader("metadata.h", ["/usr/include", "/usr/local/include"], ["/FLAC","/flac",""])
					print "\n[Checking for flac libraries]"
					flac2 = findLib("FLAC", ["/usr/lib", "/usr/local/lib"], ["/FLAC","/flac",""])
					if flac1 is None:
						print "Headers not found.  Disabling flac metadata support."
					else:
						haveflac = True
						metadata_cflags.append('-I' + flac1)
						metadata_cflags.append('-DHAVE_FLAC')
						if '-L' + flac2 not in metadata_libs_extra:
							metadata_libs_extra.append('-L' + flac2)
						metadata_libs.append('FLAC')

		if domp3 == True:
			print "\n[Checking for id3lib headers]"
			id31 = findHeader("id3.h", ["/usr/include", "/usr/local/include"], ["/id3","/id3lib",""])
			print "\n[Checking for id3lib libraries]"
			id32 = findLib("id3", ["/usr/lib", "/usr/local/lib"], ["/id3","/id3lib",""])
			print "\n[Checking for mad headers]"
			mad1 = findHeader("mad.h", ["/usr/include", "/usr/local/include"], ["/mad",""])
			print "\n[Checking for mad libraries]"
			mad2 = findLib("mad", ["/usr/lib", "/usr/local/lib"], ["/mad",""])
			print "\n[Checking for zlib libraries]"
			z1 = findLib("z", ["/usr/lib", "/usr/local/lib"], [""])
			if id31 is None or id32 is None or mad1 is None or mad2 is None or z1 is None:
				print "Headers not found.  Disabling mp3 support."
			else:
				havemp3 = True
				bossao_cfiles.append("bossogg/bossao/mp3.c")
				metadata_cflags.append('-I' + id31)
				metadata_cflags.append('-I' + mad1)
				metadata_cflags.append('-DHAVE_MP3')
				if '-L' + id32 not in metadata_libs_extra:
					metadata_libs_extra.append('-L' + id32)
				if '-L' + mad2 not in metadata_libs_extra:
					metadata_libs_extra.append('-L' + mad2)
				metadata_libs.append('id3')
				metadata_libs.append('mad')
				metadata_libs.append('z')
				metadata_libs.append('stdc++')

			if haveogg or haveflac or havemp3:
				ext_modules_list.append(Extension("bossogg.bossao._bossao",bossao_cfiles,libraries=metadata_libs,extra_compile_args=metadata_cflags,extra_link_args=metadata_libs_extra))
				packages_list.append("bossogg.bossao")
				ext_modules_list.append(Extension("bossogg.metadata._id3",["bossogg/metadata/id3.c","bossogg/metadata/id3_wrap.c","bossogg/metadata/id3lib_wrapper.cc"],libraries=metadata_libs,extra_compile_args=metadata_cflags,extra_link_args=metadata_libs_extra))
				packages_list.append("bossogg.metadata")

		if doogg == True and doripper == True:
			#Setup libs for ripper
			ripper_cflags = []
			for i in metadata_cflags:
				ripper_cflags.append(i)
			ripper_libs = []
			for i in metadata_libs:
				ripper_libs.append(i)
			ripper_libs_extra = []
			for i in metadata_libs_extra:
				ripper_libs_extra.append(i)
			havecdda = False
			havecddb = False

			print "\n[Checking for libcdda headers]"
			cdda1 = findHeader("cdda_interface.h", ["/usr/include", "/usr/local/include"], ["/cdda","/libcdda",""])
			cdda2 = findHeader("cdda_paranoia.h", ["/usr/include", "/usr/local/include"], ["/cdda","/libcdda",""])
			print "\n[Checking for libcdda libraries]"
			cdda3 = findLib("cdda_interface", ["/usr/lib", "/usr/local/lib"], ["/cdda","/libcdda",""])
			cdda4 = findLib("cdda_paranoia", ["/usr/lib", "/usr/local/lib"], ["/cdda","/libcdda",""])
			print "\n[Checking for pthread libraries]"
			pthread1 = findLib("pthread", ["/usr/lib", "/usr/local/lib"], [""])

			if cdda1 is None or cdda2 is None or cdda3 is None or cdda4 is None or pthread1 is None:
				print "Headers not found.  Disabling ripper support."
			else:
				havecdda = True
				ripper_cflags.append('-I' + cdda1)
				ripper_cflags.append('-I' + cdda2)
				if '-L' + cdda3 not in ripper_libs_extra:
					ripper_libs_extra.append('-L' + cdda3)
				if '-L' + cdda4 not in ripper_libs_extra:
					ripper_libs_extra.append('-L' + cdda4)
				if '-L' + pthread1 not in ripper_libs_extra:
					ripper_libs_extra.append('-L' + pthread1)
				ripper_libs.append('cdda_interface')
				ripper_libs.append('cdda_paranoia')
				ripper_libs.append('pthread')

			print "\n[Checking for libcddb headers]"
			cddb1 = findHeader("cddb.h", ["/usr/include", "/usr/local/include"], ["/cddb","/libcddb",""])
			print "\n[Checking for libcddb libraries]"
			cddb2 = findLib("cddb", ["/usr/lib", "/usr/local/lib"], ["/cddb","/libcddb",""])

			if cddb1 is None or cddb2 is None:
				print "Headers not found.  Disabling ripper support."
			else:
				havecddb = True
				ripper_cflags.append('-I' + cddb1)
				if '-L' + cddb2 not in ripper_libs_extra:
					ripper_libs_extra.append('-L' + cdda2)
				ripper_libs.append('cddb')

			if doripper and haveogg and havecdda and havecddb:
				ext_modules_list.append(Extension("bossogg.ripper._ripper",["bossogg/ripper/rip.c","bossogg/ripper/ripper_wrap.c","bossogg/ripper/cddb.c"],libraries=ripper_libs,extra_compile_args=ripper_cflags,extra_link_args=ripper_libs_extra))
				packages_list.append("bossogg.ripper")


#Run the setup
print
setup(name=version.getName(),
	version=version.getVersion(),
	author="Ted Kulp",
	author_email="wishy@users.sf.net",
	url="http://bossogg.sf.net/",
	ext_modules=ext_modules_list,
	packages=packages_list,
	scripts=['boshell','boimport','boss3']
)

# vim:ts=8 sw=8 noet

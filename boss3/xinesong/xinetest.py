import xinesong
import time

thelib = xinesong.xinelib()
thesong = xinesong.song()
xinesong.initlib(thelib, "auto")
#print xinesong.getAudioOutputPlugins(thelib)
#print xinesong.getFileExtensions(thelib)
#print xinesong.getMimeTypes(thelib)
xinesong.song_init(thesong, thelib)
thesong.filename = "/home/wishy/mp3/Bela Fleck & The Flecktones/Flight of the Cosmic Hippo - 1991/01 - Blu-bop.ogg"
#thesong.filename = "/home/wishy/mp3/Barenaked Ladies/Stunt - 1998/01 - One Week.ogg"
xinesong.song_open(thesong)
xinesong.song_start(thesong)
while 1:
	time.sleep(.5)

#!/usr/bin/python

import id3

#print id3.getTag("/usr/storage/Thrice/1998\ -\ Thrice\ -\ First\ Impressions/07-thrice-better_days.mp3")
print id3.getTag("broken.mp3")
print id3.getTag("/home/adam/test2.mp3")
#tmp= unicode(tmp["songname"],'latin-1')
print (id3.getTag("/home/adam/test.ogg"))
print (id3.getTag("/home/adam/test.mp3"))
#print (id3.getTag("/home/adam/test2.mp3"))
print (id3.getTag("/home/adam/bbking.flac"))


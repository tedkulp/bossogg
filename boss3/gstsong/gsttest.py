import gstsong

gstsong.gstreamer_init()

somesong = gstsong.song()

print "Song Created"

somesong.filename = "/usr/storage/music/radiohead/Radiohead - Hail to the Thief/13 - Scatterbrain.mp3"

print "Running Start"
gstsong.start(somesong)
print "Song Started"

while (1 == 1):
	gstsong.play(somesong)

gstsong.finish(somesong)

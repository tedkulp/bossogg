#!/usr/bin/python

import time
sleep = time.sleep
import bossao

song = bossao.bossao_new ();
print song
dir (song)
print song.__class__
cfg={}
bossao.bossao_start (song, cfg);
bossao.bossao_play (song, "/home/adam/test.ogg")
print bossao.bossao_filename (song)
print bossao.bossao_time_current (song)
print bossao.bossao_time_total (song)
i=0
while (bossao.bossao_finished (song) == 0):
      i=i+1
      print i
      time.sleep (1)
      if i == 10:
            i = 0
            bossao.bossao_shutdown (song)
            time.sleep(8)
            bossao.bossao_play (song, "/home/adam/test.ogg")


#bossao.bossao_play (song, "1.ogg");

#time.sleep (6);

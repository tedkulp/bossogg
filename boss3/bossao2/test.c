
#include <Python.h>

#include <stdio.h>
#include <unistd.h>
#include <ao/ao.h>

#include <mad.h>

#include <pthread.h>
#include <vorbis/vorbisfile.h>

#include "thbuf.h"
#include "bossao.h"

int main (int argc, char *argv[])
{
  song_s *testsong = NULL;
  int i = 0;

  printf("Creating new struct\n");
  testsong = bossao_new();
  printf ("preparing to start\n");
  bossao_start (testsong, NULL);
  char *filename = (char *)malloc (sizeof ("/home/adam/bbking.flac"));
  strcpy (filename, "/home/adam/test.mp3");
  char *filename2 = strdup (filename);
  printf ("filename is %s\n", filename);
  printf ("filename has %p\n", filename);
  printf ("playing first file\n");
  bossao_play (testsong, filename);
  printf ("Driver name is %s\n", bossao_driver_name (testsong));
  
  while (i < 3) {
    usleep (1000000);
    i++;
  }

  //strcpy (filename, "/home/adam/test.mp3");
  //bossao_play (testsong, filename);

  printf ("pausing\n");
  bossao_pause (testsong);
  sleep (2);
  printf ("unpausing\n");
  bossao_unpause (testsong);

  while (i < 4) {
    usleep (1000000);
    i++;
  }

  bossao_stop (testsong);
  printf ("stopped\n");
  sleep (2);
  printf ("readying the play: %s\n", filename2);
  bossao_play (testsong, filename2);
  sleep (4);
  
  /*
  printf ("playing next file\n");
  bossao_pause (testsong);
  bossao_play (testsong, argv[2]);
  bossao_unpause (testsong);
  i = 0;
  while (i < 6) {
    usleep (1000000);
    i++;
  }
	
  bossao_pause (testsong);
  printf ("paused\n");
  usleep (2000000);
  printf ("unpaused\n");
  bossao_unpause (testsong);
  i = 0;
  while (i < 3) {
    usleep (1000000);
    i++;
  }
  */
	
  bossao_shutdown (testsong);
	
  usleep (200000);
	
  printf ("closing\n");

  printf("Deleting struct\n");
  bossao_del(testsong);
  free (filename);
	
  return 0;	
}

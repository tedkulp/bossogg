
#include <stdio.h>
#include <unistd.h>
#include <ao/ao.h>

#include <mad.h>

#include <pthread.h>
#include <vorbis/vorbisfile.h>

#include "bossao.h"

int main (int argc, char *argv[])
{
  song_s *testsong = NULL;
  int i = 0;
	
  if (argv[1] == NULL || argv[2] == NULL) {
    printf ("Need 2 ogg files as arguments\n");
    return -1;
  }

  printf("Creating new struct\n");
  testsong = bossao_new();
  bossao_start (testsong, argv[1]);
			
  printf ("playing first file\n");
  while (i < 6) {
    usleep (1000000);
    i++;
  }
	
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
	
  bossao_shutdown (testsong);
	
  usleep (200000);
	
  printf ("closing\n");

  printf("Deleting struct\n");
  bossao_del(testsong);
	
  return 0;	
}

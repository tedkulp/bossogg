#include <stdio.h>
#include <unistd.h>
#include <ao/ao.h>

#include <Python.h>

#include <mad.h>

#include <pthread.h>
#include <vorbis/vorbisfile.h>

#include "bossao.h"

int main (int argc, char *argv[])
{
	int curvol, newvol;
	//mixer_s *mixer = bossao_new_mixer();
	mixer_s mixer;
	int result = bossao_open_mixer(&mixer, "/dev/mixer", "oss");
	printf("result: %d\n", result);
	curvol = bossao_getvol(&mixer);
	printf("Current Volume: %d\n", curvol);
	bossao_setvol(&mixer, 0);
	newvol = bossao_getvol(&mixer);
	printf("Current Volume: %d\n", newvol);
	bossao_setvol(&mixer, curvol);
	newvol = bossao_getvol(&mixer);
	printf("Current Volume: %d\n", newvol);
	bossao_close_mixer(&mixer);
	//bossao_del_mixer(mixer);

	return 0;	
}

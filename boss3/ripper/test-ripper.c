#include <stdio.h>
#include <cdaudio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cdda_interface.h>
#include <vorbis/vorbisenc.h>
#include <time.h>

#include "../metadata/id3.h"
//#include <cddb/cddb.h>
#include "rip.h"
#include "cddb.h"

int main (int argc, char *argv[])
{
  text_tag_s **text_tags, *text_tag;
  char **buf;
  cdrom_drive *drive = cdda_find_a_cdrom (0, buf);
  if (drive == NULL) {
	printf ("Error: [%s:%s:%d]: Couldn't find a cd drive\n", FL, FN, LN);
	exit (1);
  }
  int i;
  if (cdda_open (drive) < 0) {
	printf ("Error: [%s:%s:%d]: Couldn't open cd drive\n", FL, FN, LN);
	exit (2);
  }
  
  text_tags = query_cd (drive);

  if (text_tags != NULL) {
	printf ("The CDDB returned the following:\n\n");
	for (i = 0; text_tags[i] != NULL; i++) {
	  text_tag = text_tags[i];

	  print_text_tag (text_tags[i]);
	  printf ("\n");
	}
  }
  else
	sync ();
  
  rip (drive, text_tags, NULL);

  for (i = 0; text_tags[i] != NULL; i++) {
	free_text_tag (text_tag);
  }
  
  free (text_tags);
  cdda_close (drive);
  return 0;
}

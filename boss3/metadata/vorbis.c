#include <Python.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <string.h>

/*
 * Define me if you want to see all kinds of debugging shit.
#define DEBUG 1
*/

PyObject* getMetaData(char* filename) {

	PyObject *d;
	d = PyDict_New();

	OggVorbis_File vf;
	FILE *oggfile;
	char **comments;
	char *temp, *s1, *s2;

	oggfile = fopen(filename, "rb");
	ov_open(oggfile, &vf, NULL, 0);

#ifdef DEBUG
	printf("opened\n");
#endif

	PyDict_SetItemString(d, "bitrate", PyInt_FromLong((long)(ov_bitrate(&vf, -1)/1024)));
	PyDict_SetItemString(d, "songlength", PyInt_FromLong((long)ov_time_total(&vf, -1)));

#ifdef DEBUG
	printf("set bitrate and songlength\n");
#endif

	comments = ov_comment(&vf,-1)->user_comments;
	while(*comments) {
		temp = strdup(*comments);
#ifdef DEBUG
		printf("after temp\n");
#endif
		if (temp) {
			s1 = strtok(temp,"=");
#ifdef DEBUG
			printf("after s1\n");
#endif
			if (s1) {
#ifdef DEBUG
				printf("s1 = %s\n",s1);
#endif
				s2 = strtok(NULL,"=");
#ifdef DEBUG
				printf("after s2\n");
#endif
				if (s2) {
#ifdef DEBUG
					printf("s2 = %s\n",s2);
#endif
					PyDict_SetItemString(d, s1, PyString_FromString(s2));
#ifdef DEBUG
					printf("after setitemstring\n");
#endif
				}
			}
		}
		free(temp);
		comments++;
	}

	ov_clear(&vf);

	return d;

}

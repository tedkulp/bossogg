#ifndef _SONG_H
#define _SONG_H

#define OK 0
#define ERROR 1
#define SWITCH_SONGS 2
#define END_OF_QUEUE 3
#define EMPTY_QUEUE 4
#define INDEX_OUT_OF_RANGE 5

#define TRUE 1
#define FALSE 0

#include <gst/gst.h>

typedef struct {
        char *filename;
	char *sink;
        double playedseconds;
        double songlength;

        int initdone;

        GstElement *pipeline, *filesrc, *decoder, *audiosink;
} song;

void gstreamer_init();
int start(song *thesong);
int play(song *thesong);
int finish(song *thesong);
int restart(song *thesong);
int seek(song *thesong, int amt);

#endif

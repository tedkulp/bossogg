%module gstsong
%{
#include "gstsong.h"
#include <gst/gst.h>
%}

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


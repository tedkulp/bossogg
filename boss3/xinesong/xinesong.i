%module xinesong
%{
#include <Python.h>
#include "xinesong.h"
#include <xine.h>
%}

typedef struct {

        xine_t* xinehandle;
        xine_audio_port_t* audioport;
        xine_video_port_t* videoport;

} xinelib;

typedef struct {

        char* filename;
        double percentage;
        double timeplayed;
        double songlength;
        int songisinit;
	int streamisopen;
        xinelib *thelib;
        xine_stream_t* stream;
        char* status;

} song;

int initlib(xinelib* thelib, char *audiodriver);
void closelib(xinelib* thelib);
int song_geterror(song* thesong);
PyObject* getAudioOutputPlugins(xinelib* thelib);
void song_init(song* thesong, xinelib* thelib);
static PyObject* getMetaData(song* thesong);
int song_start(song* thesong);
int song_open(song* thesong);
int song_close(song* thesong);
int song_finish(song* thesong);
int song_updateinfo(song* thesong);
int song_play(song* thesong);
int song_pause(song* thesong);
int song_stop(song* thesong);
int song_restart(song* thesong);
int song_seek(song* thesong, int amt);
char* getFileExtensions(xinelib *thelib);
char* getMimeTypes(xinelib *thelib);
int getVolume(song *thesong);
void setVolume(song *thesong, int volume);


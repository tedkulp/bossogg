#include "xinesong.h"

/* Define me if you want to see all kinds of debugging shit.*/
#define DEBUG 1

int initlib(xinelib* thelib, char* audiodriver) {

	thelib->xinehandle = xine_new();
	xine_init(thelib->xinehandle);
	if (thelib->xinehandle == NULL) {
		return 0;
	}
#ifdef DEBUG
	printf("before audio\n");
#endif
	if (audiodriver != NULL && strcasecmp(audiodriver, "auto") == 0)
		audiodriver = NULL;
	thelib->videoport = xine_open_video_driver(thelib->xinehandle, NULL, XINE_VISUAL_TYPE_NONE, NULL);
	thelib->audioport = xine_open_audio_driver(thelib->xinehandle, audiodriver, NULL);
	if (thelib->audioport == NULL) {
		return 0;
	}
#ifdef DEBUG
	printf("audio opened\n");
#endif
	return 1;
}

void closelib(xinelib* thelib) {

	xine_close_audio_driver(thelib->xinehandle, thelib->audioport);
	xine_close_video_driver(thelib->xinehandle, thelib->videoport);
	xine_exit(thelib->xinehandle);

}

int song_geterror(song* thesong) {
	return xine_get_error(thesong->stream);
}

PyObject* getAudioOutputPlugins(xinelib* thelib) {
	const char* const* plugins = xine_list_audio_output_plugins(thelib->xinehandle);
	PyObject *d;
	d = PyList_New(0);

	while (*plugins) {
		PyList_Append(d, PyString_FromString(*plugins));
		plugins++;
	}

	return d;
}

char* getFileExtensions(xinelib *thelib) {
	return xine_get_file_extensions(thelib->xinehandle);
}

char* getMimeTypes(xinelib *thelib) {
	return xine_get_mime_types(thelib->xinehandle);
}

int getVolume(song *thesong) {
	return xine_get_param(thesong->stream, XINE_PARAM_AUDIO_VOLUME);
}

void setVolume(song *thesong, int volume) {
	if (volume >= 0 && volume <= 100) {
		xine_set_param(thesong->stream, XINE_PARAM_AUDIO_VOLUME, volume);
	}
}

PyObject* getMetaData(song* thesong) {


	int tmp_pos, tmp_pos_time, tmp_len_time;
#ifdef DEBUG
	printf("in metadata\n");
#endif
	PyObject *d;
	d = PyDict_New();

	if (xine_get_meta_info(thesong->stream, XINE_META_INFO_ARTIST) != NULL)
		PyDict_SetItemString(d, "artist", PyString_FromString(xine_get_meta_info(thesong->stream, XINE_META_INFO_ARTIST)));
	else
		PyDict_SetItemString(d, "artist", PyString_FromString(""));

	if (xine_get_meta_info(thesong->stream, XINE_META_INFO_ALBUM) != NULL)
		PyDict_SetItemString(d, "album", PyString_FromString(xine_get_meta_info(thesong->stream, XINE_META_INFO_ALBUM)));
	else
		PyDict_SetItemString(d, "album", PyString_FromString(""));

	if (xine_get_meta_info(thesong->stream, XINE_META_INFO_TITLE) != NULL)
		PyDict_SetItemString(d, "song", PyString_FromString(xine_get_meta_info(thesong->stream, XINE_META_INFO_TITLE)));
	else
		PyDict_SetItemString(d, "song", PyString_FromString(""));

	if (xine_get_meta_info(thesong->stream, XINE_META_INFO_GENRE) != NULL)
		PyDict_SetItemString(d, "genre", PyString_FromString(xine_get_meta_info(thesong->stream, XINE_META_INFO_GENRE)));
	else
		PyDict_SetItemString(d, "genre", PyString_FromString(""));

	if (xine_get_meta_info(thesong->stream, XINE_META_INFO_YEAR) != NULL)
		PyDict_SetItemString(d, "year", PyString_FromString(xine_get_meta_info(thesong->stream, XINE_META_INFO_YEAR)));
	else
		PyDict_SetItemString(d, "year", PyString_FromString(""));

	PyDict_SetItemString(d, "bitrate", PyInt_FromLong(xine_get_stream_info(thesong->stream, XINE_STREAM_INFO_AUDIO_BITRATE)));
	PyDict_SetItemString(d, "seekable", PyInt_FromLong(xine_get_stream_info(thesong->stream, XINE_STREAM_INFO_SEEKABLE)));
	PyDict_SetItemString(d, "channels", PyInt_FromLong(xine_get_stream_info(thesong->stream, XINE_STREAM_INFO_AUDIO_CHANNELS)));
	PyDict_SetItemString(d, "bits", PyInt_FromLong(xine_get_stream_info(thesong->stream, XINE_STREAM_INFO_AUDIO_BITS)));
	PyDict_SetItemString(d, "samplerate", PyInt_FromLong(xine_get_stream_info(thesong->stream, XINE_STREAM_INFO_AUDIO_SAMPLERATE)));
	PyDict_SetItemString(d, "handled", PyInt_FromLong(xine_get_stream_info(thesong->stream, XINE_STREAM_INFO_AUDIO_HANDLED)));

	if (xine_get_pos_length(thesong->stream, &tmp_pos, &tmp_pos_time, &tmp_len_time)) {
		PyDict_SetItemString(d, "songlength", PyInt_FromLong((double)tmp_len_time / (double)1000));
	}
	else {
		PyDict_SetItemString(d, "songlength", PyInt_FromLong(0));
	}

#ifdef DEBUG
	printf("size of dict: %i\n", PyDict_Size(d));
	printf("is it a dict?: %i\n", PyDict_Check(d));

	printf("returning\n");
#endif
	return d;
}

void song_init(song* thesong, xinelib* thelib) {

	/*if (thesong->songisinit == 1) {
		song_finish(thesong);
	}*/
	if (thelib->audioport != NULL) {
		if (thesong->streamisopen != 1) {
#ifdef DEBUG
			printf("before stream\n");
#endif
			thesong->stream = xine_stream_new(thelib->xinehandle, thelib->audioport, thelib->videoport);
			thesong->streamisopen = 1;
#ifdef DEBUG
			xine_set_param(thesong->stream, XINE_PARAM_VERBOSITY, XINE_VERBOSITY_DEBUG);
			printf("stream opened\n");
#endif
		}
	}
}

int song_open(song* thesong) {

#ifdef DEBUG
	printf("before open\n");
	printf("opening: %s\n", thesong->filename);
#endif
	if (xine_open(thesong->stream, thesong->filename) == 1) {
#ifdef DEBUG
		printf("after open\n");
#endif
	/*
		if (xine_play(thesong->stream, 0, 0) == 1)  {
			thesong->songisinit = 1;
			xine_set_param(thesong->stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
			thesong->status = "ok";
		}
		else {
#ifdef DEBUG
			printf("start error\n");
#endif
			thesong->songisinit = 0;
			//check error
		}
		*/
	}
	else {
#ifdef DEBUG
		printf("open error\n");
#endif
		//check error
	}
}

int song_close(song* thesong) {
	xine_close(thesong->stream);
}

int song_start(song* thesong) {

	int result = 0;
	if (xine_play(thesong->stream, 0, 0) == 0)  {
		result = 0;
		thesong->songisinit = 0;
	}
	else {
		thesong->songisinit = 1;
		xine_set_param(thesong->stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
		result = 1;
	}
	return result;

}

int song_finish(song* thesong) {

	if (thesong->songisinit == 1) {
		thesong->songisinit = 0;
		thesong->streamisopen = 0;
		xine_dispose(thesong->stream);
		return 1;
	}
	else {
		return 0;
	}
}

int song_play(song *thesong) {

	if (thesong->songisinit == 1) {
		xine_set_param(thesong->stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
	}
	return 1;

}

int song_stop(song *thesong) {
	xine_stop(thesong->stream);
	return 1;
}

int song_pause(song* thesong) {

	if (thesong->songisinit == 0) {
		song_start(thesong);
	}
	xine_set_param(thesong->stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
	return 1;

}

int song_restart(song* thesong) {

	return song_start(thesong);

}

int song_seek(song* thesong, int amt) {

	return 1;

}

int song_updateinfo(song* thesong) {
		int tmp_pos, tmp_pos_time, tmp_len_time;
	if (thesong->songisinit == 1) {
		int tmpstatus = xine_get_status(thesong->stream);
		if (tmpstatus == XINE_STATUS_IDLE) {
			thesong->status = "idle";
		}
		else if (tmpstatus == XINE_STATUS_STOP) {
			thesong->status = "stop";
		}
		else if (tmpstatus == XINE_STATUS_PLAY) {
			thesong->status = "play";
		}
		else if (tmpstatus == XINE_STATUS_QUIT) {
			thesong->status = "quit";
		}

		if (xine_get_pos_length(thesong->stream, &tmp_pos, &tmp_pos_time, &tmp_len_time) && tmp_pos >= 0) {
			thesong->percentage = (double)((double)tmp_pos / (double)65535.0 * (double)100);
			thesong->timeplayed = (double)((double)tmp_pos_time / (double)1000);
			thesong->songlength = (double)((double)tmp_len_time / (double)1000);
		}
		else {
			thesong->percentage = 0;
			thesong->timeplayed = 0;
			thesong->songlength = 0;
		}
	}
	return 1;

}


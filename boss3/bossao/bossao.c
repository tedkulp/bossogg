/*
 * Boss Ogg - A Music Server
 * (c)2003 by Adam Torgerson (may1937@users.sf.net)
 * This project's homepage is: http://bossogg.wishy.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ao/ao.h>
#include <Python.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bossao.h"

#ifdef HAVE_SHOUT
#include <shout/shout.h>
#include "shout.h"
#endif

#ifdef HAVE_VORBIS
#include <vorbis/vorbisfile.h>
#include "ogg.h"
#endif

#ifdef HAVE_MP3
#include <mad.h>
#include "mp3.h"
#endif

#ifdef HAVE_FLAC
#include <FLAC/file_decoder.h>
#include "flac.h"
#endif

#ifdef HAVE_OSS_MIXER
#include "oss_mixer.h"
#endif

song_s *bossao_new(void)
{
  song_s *newsong = (song_s *)malloc (sizeof (song_s));
  newsong->songlib = (songlib_s *)malloc (sizeof (songlib_s));
#ifdef HAVE_SHOUT
  newsong->encoder = NULL;
#endif
  newsong->device = NULL;
  newsong->filename = NULL;
  newsong->type = -1;
  newsong->done = 0;
  newsong->shutdown = 1;
  newsong->newfile = 0;
  newsong->seek = 0.0;
  newsong->free = 1;
  newsong->device = NULL;
  pthread_mutex_init (&newsong->mutex, NULL);

#ifdef HAVE_VORBIS
  newsong->songlib->vf = NULL;
#endif
#ifdef HAVE_MP3
  newsong->songlib->mp3 = NULL;
#endif
#ifdef HAVE_FLAC
  newsong->songlib->flac = NULL;
#endif

  return newsong;
}

void bossao_del(song_s *song)
{
  free (song->songlib);
  if (song->filename != NULL)
    free (song->filename);
#ifdef HAVE_SHOUT
  free (song->encoder);
#endif
  free (song);
}

ao_device *bossao_open (void)
{
  int driver_id;
  ao_sample_format format;

  format.bits = 16;
  format.rate = 44100;
  format.channels = 2;
  format.byte_format = AO_FMT_LITTLE;
  
  ao_device *device = NULL;
  
  ao_initialize ();
  
  driver_id = ao_default_driver_id ();
  device = ao_open_live (driver_id, &format, NULL);
  if (device == NULL) {
    printf ("Problem opening audio output device\n");
    return NULL;
  }
  
  return device;
}

int identify_media (song_s *song, char *filename)
{
  int type = -1;
  FILE *file;

  file = fopen (filename, "r");

#ifdef HAVE_VORBIS
  if (identify_ogg (song, file) == 0) {
    type = VORBIS;
  }
#endif
#ifdef HAVE_MP3
  if (identify_mp3 (song, file) == 0) {
    type = MP3;
  }
#endif
#ifdef HAVE_FLAC
  if (identify_flac (song, file) == 0) {
    type = FLAC;
  }
#endif

  return type;
}

static int prepare_media (song_s *song)
{
  int type = identify_media (song, song->filename);
  int ret = 0;

  song->type = type;

#ifdef HAVE_VORBIS
  if (song->type == VORBIS) {
    if (prepare_ogg (song, song->filename) == NULL) {
      ret = -1;
    }
  }
#endif
#ifdef HAVE_MP3
  if (song->type == MP3) {
    if ((void*)prepare_mp3 (song, song->filename) == NULL) {
      ret = -1;
    }
  }
#endif
#ifdef HAVE_FLAC
  if (song->type == FLAC) {
    if (prepare_flac (song, song->filename) == NULL) {
      ret = -1;
    }
  }
#endif
  if (ret != 0) {
    printf ("Problem openining media \'%s\'\n", song->filename);
    return -1;
  }

  song->newfile = 0;

  return 0;
}

static int destroy_media (song_s *song)
{
#ifdef HAVE_VORBIS
  if (song->type == VORBIS)
    destroy_ogg (song);
#endif
#ifdef HAVE_MP3
  if (song->type == MP3) {
    destroy_mp3 (song);
  }
#endif
#ifdef HAVE_FLAC
  if (song->type == FLAC) {
    destroy_flac (song);
  }
#endif

  song->done = 1;
  song->pause = 1;
  song->shutdown = 1;

  return 0;
}

static double time_total_media (song_s *song)
{
  double ret = 0.0;

  if (song->shutdown != 1) {
#ifdef HAVE_VORBIS
    if (song->type == VORBIS)
      ret = time_total_ogg (song);
#endif
#ifdef HAVE_MP3
    if (song->type == MP3)
      ret = time_total_mp3 (song);
#endif
#ifdef HAVE_FLAC
    if (song->type == FLAC)
      ret = time_total_flac (song);
#endif
  } else
    ret = -1;

  return ret;
}

static double time_current_media (song_s *song)
{
  double ret = 0.0;

  if (song->shutdown != 1) {
#ifdef HAVE_VORBIS
    if (song->type == VORBIS)
      ret = time_current_ogg (song);
#endif
#ifdef HAVE_MP3
    if (song->type == MP3) {
      ret = time_current_mp3 (song);
    }
#endif
#ifdef HAVE_FLAC
    if (song->type == FLAC) {
      ret = time_current_flac (song);
    }
#endif
  } else
    ret = -1;

  return ret;
}

/* must be called from within a lock */
static long chunk_play_media (song_s *song, char *buffer, ao_device *device)
{
  long ret = 0;

#ifdef HAVE_VORBIS
  if (song->type == VORBIS)
    ret = chunk_play_ogg (song, buffer);
#endif
#ifdef HAVE_MP3
  if (song->type == MP3)
    ret = chunk_play_mp3 (song, buffer);
#endif
#ifdef HAVE_FLAC
  if (song->type == FLAC)
    ret = chunk_play_flac (song, buffer);
#endif

  return ret;
}

static int seek_media (song_s *song)
{
  int ret = 0;

  if (song->shutdown != 1) {
#ifdef HAVE_VORBIS
    if (song->type == VORBIS)
      seek_ogg (song);
#endif
#ifdef HAVE_MP3
    if (song->type == MP3)
      seek_mp3 (song);
#endif
#ifdef HAVE_FLAC
    if (song->type == FLAC)
      seek_flac(song);
#endif
  } else
    ret = -1;

  return ret;
}

static void *bossao_thread (void *arg)
{
  song_s *song = (song_s*)arg;
  double length;
  //ao_device *device = song->device;

  int len;
  
  char *buffer = NULL;
  if (song->filename != NULL)
    prepare_media (song);
  long bytes;
	
  buffer = (char *)malloc (sizeof (char) * BUF_SIZE);
  while (1) {
    int pause = 0;
    double seek = 0;
    int shutdown = 0;
    int newfile = 0;
    int tofree = 0;

    pthread_mutex_lock (&song->mutex);
    
    pause = song->pause;
    seek = song->seek;
    shutdown = song->shutdown;
    newfile = song->newfile;
    tofree = song->free;

    pthread_mutex_unlock (&song->mutex);

    if (newfile == 1) {
      pthread_mutex_lock (&song->mutex);
      prepare_media (song);
      length = time_total_media (song);
      song->length = length;
      pthread_mutex_unlock (&song->mutex);
    }

    if (seek != 0.0) {
      pthread_mutex_lock (&song->mutex);
      seek_media (song);
      pthread_mutex_unlock (&song->mutex);
    }
    if (shutdown == 0) {
      if (pause == 0) {

	pthread_mutex_lock (&song->mutex);

	double time = time_current_media (song);
	song->curtime = time;

        pthread_mutex_unlock (&song->mutex);

        pthread_mutex_lock (&song->mutex);

	if (chunk_play_media (song, buffer, song->device) == 0) {
	  destroy_media (song);
	  free (song->filename);
	  song->filename = NULL;
	  song->free = 1;
	}

	pthread_mutex_unlock (&song->mutex);

      }
      else {
	usleep (50000);
      }
    } else {

      pthread_mutex_lock (&song->mutex);
      if (song->filename != NULL) {
	destroy_media (song);
	free (song->filename);
	song->filename = NULL;
	song->free = 1;
      }
      pthread_mutex_unlock (&song->mutex);

      /* doesn't work 
      if (song->device != NULL) {
	pthread_mutex_unlock (&song->mutex);
	bossao_close (song);
	pthread_mutex_lock (&song->mutex);
      }
      */

      usleep (50000);
    }
  }

  free (buffer);
  bossao_close (song);
  //ao_shutdown ();
  pthread_detach (pthread_self ());
  
  return NULL;
}

int bossao_seek (song_s *song, double new_sec)
{
  int ret;

  double new_pos = new_sec + song->curtime;

  if (new_pos > 0 && new_pos < song->length) {
    song->seek = new_pos;
    ret = 0;
  }
  else
    ret = -1;

  return ret;
}

static ao_device *bossao_begin (song_s *song)
{
  PyObject *temp;
  int enable = 0;

  song->shutdown = 0;
  song->pause = 0;
  song->newfile = 1;
  song->device = bossao_open ();
#ifdef HAVE_SHOUT
  if (song->cfgparser != NULL) {
    temp = PyObject_CallMethod (song->cfgparser, "get", "s,s", "enable", "SHOUT");
    if (temp != NULL) {
      enable = atoi (PyString_AsString (temp));
      Py_DECREF (temp);
    }
  }
  
  if (enable) {
    song->st = bossao_shout_init (song->cfgparser);
    if (song->st != NULL)
      shout_encoder_init (song, song->cfgparser);
  }
#endif  
  return song->device;
}

int bossao_start (song_s *song, PyObject *cfgparser)
{
  pthread_t thread;
  //PyObject *temp;
  //int enable = 0;
  /*
  if (filename != NULL) {
    song->filename = strdup (filename);
    song->seek = 0.0;
    song->shutdown = 0;
    song->pause = 0;
    song->newfile = 1;
    song->device = bossao_open ();
#ifdef HAVE_SHOUT
    if (cfgparser != NULL) {
      temp = PyObject_CallMethod (cfgparser, "get", "s,s", "enable", "SHOUT");
      if (temp != NULL) {
	enable = atoi (PyString_AsString (temp));
	Py_DECREF (temp);
      }
    }
    
    if (enable) {
      song->st = bossao_shout_init (cfgparser);
      if (song->st != NULL)
	shout_encoder_init (song, cfgparser);
    }
#endif  
    
    if (song->device != NULL)
      pthread_create (&thread, NULL, bossao_thread, (void*)song);
  } else {
    printf ("%s called with NULL filename!\n", FN);
    return NULL;
  }
  */

  song->shutdown = 1;
  song->pause = 1;
  song->cfgparser = cfgparser;

  pthread_create (&thread, NULL, bossao_thread, (void*)song);

  return 0;	
}

void bossao_shutdown (song_s *song)
{
  pthread_mutex_lock (&song->mutex);

  song->shutdown = 1;
  
  pthread_mutex_unlock (&song->mutex);
}

void bossao_play_chunk (song_s *song, unsigned char *buffer, int size)
{
  ao_play (song->device, buffer, size);

#ifdef HAVE_SHOUT
  if (song->encoder != NULL) {
    if (size)
      shout_encode_chunk (song, buffer, size);
    shout_sync (song->st);
  }
#endif
}

int bossao_play (song_s *song, char *filename)
{

  pthread_mutex_lock (&song->mutex);

  /* doesn't work (ao_close)
  printf ("in play\n");

  if (song->device == NULL) {
    printf ("opening\n");
    song->device = bossao_open ();
    printf ("opened\n");
    if (song->device == NULL) {
      printf ("suck balls\n");
      pthread_mutex_unlock (&song->mutex);
      return -1;
    }
  }

  printf ("we're good\n");
  */

  if (song->filename != NULL)
    free (song->filename);
  song->filename = strdup (filename);
  song->shutdown = 0;
  song->newfile = 1;
  song->done = 0;
  song->pause = 0;

  if (song->device == NULL) {
    song->device = bossao_begin (song);
    prepare_media (song);
  }
  
  pthread_mutex_unlock (&song->mutex);	
  
  return 0;	
}

int bossao_pause (song_s *song)
{
  pthread_mutex_lock (&song->mutex);
  
  if (song->pause)
    song->pause = 0;
  else 
    song->pause = 1;	
  
  pthread_mutex_unlock (&song->mutex);
  
  return song->pause;
}

int bossao_finished (song_s *song)
{
  int ret;
  pthread_mutex_lock (&song->mutex);

  ret = song->done;

  pthread_mutex_unlock (&song->mutex);

  return ret;
}

int bossao_unpause (song_s *song)
{
  pthread_mutex_lock (&song->mutex);
  
  song->pause = 0;
  
  pthread_mutex_unlock (&song->mutex);
  
  return 0;
}

double bossao_time_current (song_s *song)
{
  double ret = -1;
  int shutdown = 0;

  pthread_mutex_lock (&song->mutex);
  shutdown = song->shutdown;
  if (song->songlib == NULL) {
    printf ("bossao averting NULL pointer\n");
    shutdown = 1;
  }

  if (!shutdown) {
    ret = time_current_media (song);
  }

  pthread_mutex_unlock (&song->mutex);

  return ret;
}

double bossao_time_total (song_s *song)
{
  double ret = -1;
  int shutdown = 0;

  pthread_mutex_lock (&song->mutex);
  shutdown = song->shutdown;
  if (song->songlib == NULL) {
    printf ("bossao averting NULL pointer\n");
    shutdown = 1;
  }
 
  if (!shutdown) {
    ret = time_total_media (song);
  }

  pthread_mutex_unlock (&song->mutex);

  return ret;
}

char *bossao_filename (song_s *song)
{
  char *filename = NULL;
  pthread_mutex_lock (&song->mutex);

  if (!song->shutdown) {
    filename = song->filename;
  }

  pthread_mutex_unlock (&song->mutex);
  return filename;
}

int bossao_close (song_s *song)
{
  
  pthread_mutex_lock (&song->mutex);

  printf ("closing!?\n");

  if (song->device != NULL) {
    ao_close (song->device);
    ao_shutdown ();
    song->device = NULL;
  }

#ifdef HAVE_SHOUT
  if (song->encoder != NULL) {
    shout_clear (song);
  }
#endif

  pthread_mutex_unlock (&song->mutex);

  return 0;	
}

char *bossao_driver_name (song_s *song)
{
  char *ret = NULL;

  pthread_mutex_lock (&song->mutex);

  if (song->device != NULL){
    ao_info *info;
    info = ao_driver_info (song->device->driver_id);
    ret = info->short_name;
  } 

  pthread_mutex_unlock (&song->mutex);

  return ret;
}

int bossao_open_mixer (mixer_s *mixer, char* mixerloc, char* mixertype)
{
  mixer->type = NO_MIXER;
#ifdef HAVE_OSS_MIXER
  if (strcmp(mixertype, "oss") == 0) {
    mixer->type = OSS_MIXER;
    mixer->fd = oss_open_mixer(mixerloc);
  }
#endif
  if (strcmp(mixertype, "alsa") == 0) {
    mixer->type = ALSA_MIXER;
  }
  return mixer->type;
}

int bossao_close_mixer (mixer_s *mixer)
{
  int result = 0;
  switch(mixer->type) {
#ifdef HAVE_OSS_MIXER
  case OSS_MIXER:
    if (mixer->fd > -1) {
      result = oss_close_mixer(mixer->fd);
    }
#endif
  default:
    result = 0;
  }
  return result;
}

int bossao_getvol (mixer_s *mixer)
{
  int result = 0;
#ifdef HAVE_OSS_MIXER
  if (mixer->type == OSS_MIXER) {
    if (mixer->fd > -1) {
      result = oss_getvol(mixer->fd);
    }
  }
#endif
  return result;
}

int bossao_setvol (mixer_s *mixer, int vol)
{
  int result = 0;
#ifdef HAVE_OSS_MIXER
  if (mixer->type == OSS_MIXER) {
    if (mixer->fd > -1) {
      result = oss_setvol(mixer->fd, vol);
    }
  }
#endif
  return result;
}

mixer_s *bossao_new_mixer(void)
{
  mixer_s *newmixer = (mixer_s *)malloc (sizeof (mixer_s));
  newmixer->type = NO_MIXER;
  newmixer->fd = -1;
  return newmixer;
}

void bossao_del_mixer(mixer_s *mixer)
{
  if (mixer != NULL)
    free(mixer);
}

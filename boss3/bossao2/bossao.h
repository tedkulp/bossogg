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

//#ifndef _BOSSAO_H
//#define _BOSSAO_H

#include <pthread.h>

#define FN __FUNCTION__
#define LN __LINE__
#define FL __FILE__

#define VORBIS 0
#define MP3 1
#define FLAC 2

#define NO_MIXER 0
#define OSS_MIXER 1
#define ALSA_MIXER 2

#define BUF_SIZE 4000
#define BUF_CHUNKS 256

#define RATE 44100

#ifdef __cplusplus
extern "C" {
#endif
  
  typedef struct songlib_t {
#ifdef HAVE_VORBIS
    struct OggVorbis_File *vf;
#endif
#ifdef HAVE_MP3
    struct mp3_t *mp3;
#endif
#ifdef HAVE_FLAC
    struct FLAC__FileDecoder *flac;
#endif
  } songlib_s;
  
  typedef struct song_t {
    pthread_mutex_t mutex;
    thbuf_s *thbuf;
    double length;
    double curtime;
    char *filename;
    double seek;
    int shutdown;
    //int pause;
    int newfile;
    int free;
    songlib_s *songlib;
    int type;
    ao_device *device;
    int done;
#ifdef HAVE_SHOUT
    struct shout *st;
    struct encoder_t *encoder;
#endif
    PyObject *cfgparser;
  } song_s;
  
  typedef struct mixer_t {
    int type;
    int fd;
    char *device_name;
  } mixer_s;
 
  song_s *bossao_new(void);
  void bossao_del(song_s *song);
  ao_device *bossao_open (void);
  int bossao_close (song_s *song);
  int bossao_start (song_s *song, PyObject *cfgparser);
  int bossao_seek (song_s *song, double secs);
  void bossao_shutdown (song_s *song);
  void bossao_stop (song_s *song);
  int bossao_play (song_s *song, char *filename);
  int bossao_pause (song_s *song);
  int bossao_unpause (song_s *song);
  int bossao_finished (song_s *song);
  double bossao_time_total (song_s *song);
  double bossao_time_current (song_s *song);
  char *bossao_filename (song_s *song);
  mixer_s *bossao_new_mixer(void); /*Not really needed... yet*/
  int bossao_open_mixer(mixer_s *mixer, char* mixerloc, char* mixertype);
  int bossao_close_mixer(mixer_s *mixer);
  int bossao_getvol(mixer_s *mixer);
  int bossao_setvol(mixer_s *mixer, int vol);
  void bossao_del_mixer(mixer_s *mixer); /*Ditto on this one*/
  char *bossao_driver_name (song_s *song);
  void bossao_play_chunk (song_s *song, unsigned char *buffer, int size);
  
  //#endif
#ifdef __cplusplus
}
#endif

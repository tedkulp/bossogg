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

#import <Python.h>

#import "common.h"
#import "thbuf.h"

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
  
   typedef struct song_t {
      thbuf_t *thbuf;
      double length;
      double curtime;
      char *filename;
      double seek;
      int shutdown;
      //int pause;
      int newfile;
      int free;
      int type;
      int done;
#ifdef HAVE_SHOUT
      struct shout *st;
      struct encoder_t *encoder;
#endif
      PyObject *cfgparser;
   } song_s;
   
   void bossao_new(PyObject *cfgparser);
   void bossao_free(void);
   
   void bossao_open (PyObject *cfgparser);
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
   char *bossao_driver_name (song_s *song);
   void bossao_play_chunk (song_s *song, unsigned char *buffer, int size);
   void bossao_thread_init (void);
   
   //#endif
#ifdef __cplusplus
}
#endif

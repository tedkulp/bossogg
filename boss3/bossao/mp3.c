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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mad.h>
#include <ao/ao.h>
#include <Python.h>

#include <config.h>
#include "bossao.h"
#include "mp3.h"

struct audio_dither {
  mad_fixed_t error[3];
  mad_fixed_t random;
};

static void mp3_init (mp3_s *mp3)
{
  mp3->out_ptr = mp3->out_buf;
  mp3->out_buf_end = mp3->out_buf + BUF_SIZE;
  mp3->frame_count = 0;
  mp3->status = 0;
  mp3->start = 0;
  mad_stream_init (&mp3->stream);
  mad_frame_init (&mp3->frame);
  mad_synth_init (&mp3->synth);
  mad_timer_reset (&mp3->timer);
}

static void mp3_finish (mp3_s *mp3)
{
  mad_synth_finish (&mp3->synth);
  mad_frame_finish (&mp3->frame);
  mad_stream_finish (&mp3->stream);

  fclose (mp3->file);
}

static unsigned long prng (unsigned long state) {
  return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

static signed long audio_linear_dither (unsigned int bits, mad_fixed_t sample,
					struct audio_dither *dither)
{
  unsigned int scalebits;
  mad_fixed_t output, mask, random;

  enum {
    MIN = -MAD_F_ONE,
    MAX = MAD_F_ONE - 1
  };

  sample += dither->error[0] - dither->error[1] + dither->error[2];

  dither->error[2] = dither->error[1];
  dither->error[1] = dither->error[0] / 2;

  output = sample + (1L << (MAD_F_FRACBITS + 1 - bits - 1));

  scalebits = MAD_F_FRACBITS + 1 - bits;
  mask = (1L << scalebits) - 1;

  random = prng (dither->random);
  output += (random & mask) - (dither->random & mask);

  dither->random = random;

  if (output > MAX) {
    output = MAX;
    if (sample > MAX)
      sample = MAX;
  } else if (output < MIN) {
    output = MIN;
    if (sample < MIN)
      sample = MIN;
  }
  output &= ~mask;

  dither->error[0] = sample - output;

  return output >> scalebits;
}

static int mp3_fill_input (mp3_s *mp3)
{
  size_t size, remaining;
  unsigned char *read_start;

  if (mp3->stream.next_frame != NULL) {
    remaining = mp3->stream.bufend - mp3->stream.next_frame;
    memmove (mp3->read_buf, mp3->stream.next_frame, remaining);
    read_start = mp3->read_buf + remaining;
    size = READ_BUFFER_SIZE - remaining;
  } else {
    size = READ_BUFFER_SIZE;
    read_start = mp3->read_buf;
    remaining = 0;
  }

  size = fread (read_start, 1, size, mp3->file);
  if (size <= 0)
    return -1;

  mad_stream_buffer (&mp3->stream, mp3->read_buf, size + remaining);
  mp3->stream.error = 0;

  return 0;
}

static int mp3_decode_frame (mp3_s *mp3)
{
  if (mp3->stream.buffer == NULL || mp3->stream.error == MAD_ERROR_BUFLEN) {
    if (mp3_fill_input (mp3) < 0) {
      return DECODE_BREAK;
    }
  }

  if (mad_frame_decode (&mp3->frame, &mp3->stream)) {
    if (MAD_RECOVERABLE (mp3->stream.error)) {
      return DECODE_CONT;
    } else {
      if (mp3->stream.error == MAD_ERROR_BUFLEN) {
	return DECODE_CONT;
      } else {
	printf ("Unrecoverable frame error: \'%s\'\n", mad_stream_errorstr (&mp3->stream));
	mp3->status = 1;
	return DECODE_BREAK;
      }
    }
  }

  mp3->frame_count++;

  return DECODE_OK;
}

static int mp3_open (mp3_s *mp3, char *filename)
{
  int ret;
  struct stat filestat;

  mp3_init (mp3);
  if ((mp3->file = fopen (filename, "r")) <= 0) {
    printf ("Problem opening file \'%s\'\n", filename);
    return -1;
  }
  fstat (fileno (mp3->file), &filestat);
  while ((ret = mp3_decode_frame (mp3)) == DECODE_CONT)
    ; //nothing
  mp3->total_time = (filestat.st_size * 8.0) / mp3->frame.header.bitrate;

  return ret;
}

static int mp3_read (song_s *song, char *buffer)
{
  static int i;
  static int ret;
  static struct audio_dither dither;
  static char buffer2[BUF_SIZE];
  static char *out_ptr = buffer2;
  static char *out_buf_end = buffer2 + BUF_SIZE;
  mp3_s *mp3 = song->songlib->mp3;

  mad_timer_add (&mp3->timer, mp3->frame.header.duration);
  mad_synth_frame (&mp3->synth, &mp3->frame);
  mp3->elapsed_time = ((float)mad_timer_count (mp3->timer, MAD_UNITS_MILLISECONDS))/1000.0;
  
  for (i = 0; i < mp3->synth.pcm.length; i++) {
    signed int sample;
    sample = (signed int)audio_linear_dither (16, mp3->synth.pcm.samples[0][i], &dither);
   
    *(out_ptr++) = sample & 0xff;
    *(out_ptr++) = sample >> 8;

    if (MAD_NCHANNELS (&(mp3->frame).header) == 2) {
      sample = (signed int) audio_linear_dither (16, mp3->synth.pcm.samples[1][i], &dither);

      *(out_ptr++) = sample & 0xff;
      *(out_ptr++) = sample >> 8;
    }

    if (out_ptr == out_buf_end) {
      memcpy (buffer, buffer2, BUF_SIZE);
      bossao_play_chunk (song, buffer, BUF_SIZE);
      out_ptr = buffer2;
    }
  }
  

  while ((ret = mp3_decode_frame (mp3)) == DECODE_CONT)
    ;
  
  return ret;
}


void *prepare_mp3 (song_s *song, char *filename)
{
  song->songlib->mp3 = (mp3_s *)malloc (sizeof (mp3_s));

  if (mp3_open (song->songlib->mp3, filename) == -1) {
    free (song->songlib->mp3);
    return NULL;
  }
  return song->songlib->mp3;
}

int destroy_mp3 (song_s *song)
{
  mp3_finish (song->songlib->mp3);
  free (song->songlib->mp3);

  return 0;
}

long chunk_play_mp3 (song_s *song, char *buffer)
{
  if (mp3_read (song, buffer) == DECODE_BREAK)
    return 0;

  return BUF_SIZE;
}

int seek_mp3 (song_s *song)
{
  //printf ("seek unimplemented for mp3\n");
  return 0;
}

double time_total_mp3 (song_s *song)
{
  return song->songlib->mp3->total_time;
}

double time_current_mp3 (song_s *song)
{
  double number = (double)mad_timer_count (song->songlib->mp3->timer, MAD_UNITS_MILLISECONDS) / 1000.0;
  return number;
}

int identify_mp3 (song_s *song, FILE *file)
{
  int ret = -1;

  //printf ("identifying mp3 %s\n", song->filename);
  
  int len = strlen (song->filename);
  char *ptr = &song->filename[len - 3];

  if (strncmp (ptr, "mp3", 3) == 0)
    ret = 0;

  return ret;
}

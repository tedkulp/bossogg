/*
 * Boss Ogg - A Music Server
 * (c)2004 by Adam Torgerson (adam.torgerson@colorado.edu)
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

 * mp3
 
*/

#define INPUT_IMPLEMENTATION
#import "input_plugin.h"
#import <mad.h>

#define DECODE_BREAK -2
#define DECODE_CONT -1
#define DECODE_OK 0
#define READ_BUFFER_SIZE (4*8192)

static gchar *plugin_name="mp3";

static gdouble mp3_total_time = 0.0;
static gdouble mp3_elapsed_time = 0.0;
static gint mp3_status;
static gint mp3_start;
static gint mp3_frame_count;
static FILE *mp3_file;
static unsigned char mp3_read_buf[READ_BUFFER_SIZE];

static struct mad_stream mp3_stream;
static struct mad_frame mp3_frame;
static struct mad_synth mp3_synth;
static mad_timer_t mp3_timer;

struct audio_dither {
   mad_fixed_t error[3];
   mad_fixed_t random;
};

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

int input_identify (gchar *filename)
{
   int ret = 0;

   if (filename == NULL)
      return 0;

   int len = strlen (filename);
   gchar *ptr = &filename[len - 3];

   if (strncmp (ptr, "mp3", 3) == 0)
      ret = 1;
   
   return ret;
}

int input_seek (song_s *song, gdouble length)
{
   LOG ("Unimplemented");
   return 0;
}

double input_time_total (song_s *song)
{
   return mp3_total_time;
}

double input_time_current (song_s *song)
{
   double number = (double)mad_timer_count (mp3_timer, MAD_UNITS_MILLISECONDS) / 1000.0;
   return number;
}

static int mp3_fill_input (void)
{
   size_t size, remaining;
   unsigned char *read_start;

   if (mp3_stream.next_frame != NULL) {
      remaining = mp3_stream.bufend - mp3_stream.next_frame;
      memmove (mp3_read_buf, mp3_stream.next_frame, remaining);
      read_start = mp3_read_buf + remaining;
      size = READ_BUFFER_SIZE - remaining;
   } else {
      size = READ_BUFFER_SIZE;
      read_start = mp3_read_buf;
      remaining = 0;
   }

   size = fread (read_start, 1, size, mp3_file);
   if (size <= 0)
      return -1;

   mad_stream_buffer (&mp3_stream, mp3_read_buf, size + remaining);
   mp3_stream.error = 0;

   return 0;
}

static int mp3_decode_frame (void)
{
   if (mp3_stream.buffer == NULL || mp3_stream.error == MAD_ERROR_BUFLEN) {
      if (mp3_fill_input () < 0) {
	 return DECODE_BREAK;
      }
   }

   if (mad_frame_decode (&mp3_frame, &mp3_stream)) {
      if (MAD_RECOVERABLE (mp3_stream.error)) {
	 return DECODE_CONT;
      } else {
	 if (mp3_stream.error == MAD_ERROR_BUFLEN) {
	    return DECODE_CONT;
	 } else {
	    LOG ("Unrecoverable frame error: '%s'", mad_stream_errorstr (&mp3_stream));
	    mp3_status = 1;
	    return DECODE_BREAK;
	 }
      }
   }

   mp3_frame_count++;

   return DECODE_OK;
}

static int mp3_read (song_s *song, char *buffer)
{
   static int i;
   static int ret;
   static struct audio_dither dither;
   static unsigned char buffer2[BUF_SIZE];
   static unsigned char *out_ptr = buffer2;
   static unsigned char *out_buf_end = buffer2 + BUF_SIZE;

   mad_timer_add (&mp3_timer, mp3_frame.header.duration);
   mad_synth_frame (&mp3_synth, &mp3_frame);
   mp3_elapsed_time = ((float)mad_timer_count (mp3_timer, MAD_UNITS_MILLISECONDS))/1000.0;
  
   for (i = 0; i < mp3_synth.pcm.length; i++) {
      signed int sample;
      sample = (signed int)audio_linear_dither (16, mp3_synth.pcm.samples[0][i], &dither);
   
      *(out_ptr++) = sample & 0xff;
      *(out_ptr++) = sample >> 8;

      if (MAD_NCHANNELS (&(mp3_frame).header) == 2) {
	 sample = (signed int) audio_linear_dither (16, mp3_synth.pcm.samples[1][i], &dither);

	 *(out_ptr++) = sample & 0xff;
	 *(out_ptr++) = sample >> 8;
      }

      if (out_ptr == out_buf_end) {
	 memcpy (buffer, buffer2, BUF_SIZE);
	 //output_plugin_write_chunk_all (NULL, buffer, BUF_SIZE);
	 out_ptr = buffer2;
      }
   }
  

   while ((ret = mp3_decode_frame ()) == DECODE_CONT)
      ;
  
   return ret;
}

char *input_play_chunk (song_s *song, gint *size)
{
   gchar *buffer = (gchar *)g_malloc (BUF_SIZE);

   if (mp3_read (song, buffer) == DECODE_BREAK)
      return NULL;

   *size = BUF_SIZE;
   
   return buffer;
}

int input_open (song_s *song, gchar *filename)
{
   struct stat filestat;
   int ret;
   
   mp3_frame_count = 0;
   mp3_status = 0;
   mp3_start = 0;
   
   mad_stream_init (&mp3_stream);
   mad_frame_init (&mp3_frame);
   mad_timer_reset (&mp3_timer);

   if ((mp3_file = fopen (filename, "r")) <= 0) {
      LOG ("Problem opening file '%s'", filename);
      return -1;
   }
   fstat (fileno (mp3_file), &filestat);
   while ((ret = mp3_decode_frame ()) == DECODE_CONT)
      ; /* do nothing */

   mp3_total_time = (filestat.st_size * 8.0) / mp3_frame.header.bitrate;

   return 0;
}

int input_close (song_s *song)
{
   mad_synth_finish (&mp3_synth);
   mad_frame_finish (&mp3_frame);
   mad_stream_finish (&mp3_stream);
   
   fclose (mp3_file);
   
   return 0;
}

gchar *input_name (void)
{
   return plugin_name;
}

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

#import <mad.h>
#import "input_plugin.h"
#import "bossao.h"

#define DECODE_BREAK -2
#define DECODE_CONT -1
#define DECODE_OK 0
#define READ_BUFFER_SIZE (4*8192)

static gchar *plugin_name="mp3";

typedef struct private_mp3_t {
   gdouble mp3_total_time;
   gdouble mp3_elapsed_time;
   gint mp3_status;
   gint mp3_start;
   gint mp3_frame_count;
   FILE *mp3_file;
   guchar mp3_read_buf[READ_BUFFER_SIZE];

   struct mad_stream mp3_stream;
   struct mad_frame mp3_frame;
   struct mad_synth mp3_synth;
   mad_timer_t mp3_timer;
} private_mp3_s;

struct audio_dither {
   mad_fixed_t error[3];
   mad_fixed_t random;
};

static unsigned long prng (unsigned long state) {
   return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

static signed long audio_linear_dither (guint bits, mad_fixed_t sample,
					struct audio_dither *dither)
{
   guint scalebits;
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

gint _input_identify (gchar *filename)
{
   gint ret = 0;

   if (filename == NULL)
      return 0;

   gint len = strlen (filename);
   gchar *ptr = &filename[len - 3];

   if (strncmp (ptr, "mp3", 3) == 0)
      ret = 1;
   
   return ret;
}

gint _input_seek (song_s *song, gdouble length)
{
   private_mp3_s *p_mp3 = (private_mp3_s *)song->private;
   LOG ("Unimplemented");
   return 0;
}

gdouble _input_time_total (song_s *song)
{
   private_mp3_s *p_mp3 = (private_mp3_s *)song->private;
   return p_mp3->mp3_total_time;
}

gdouble _input_time_current (song_s *song)
{
   private_mp3_s *p_mp3 = (private_mp3_s *)song->private;
   gdouble number = (double)mad_timer_count (p_mp3->mp3_timer, MAD_UNITS_MILLISECONDS) / 1000.0;
   return number;
}

static gint mp3_fill_input (private_mp3_s *p_mp3)
{
   size_t size, remaining;
   guchar *read_start;

   if (p_mp3->mp3_stream.next_frame != NULL) {
      remaining = p_mp3->mp3_stream.bufend - p_mp3->mp3_stream.next_frame;
      memmove (p_mp3->mp3_read_buf, p_mp3->mp3_stream.next_frame, remaining);
      read_start = p_mp3->mp3_read_buf + remaining;
      size = READ_BUFFER_SIZE - remaining;
   } else {
      size = READ_BUFFER_SIZE;
      read_start = p_mp3->mp3_read_buf;
      remaining = 0;
   }

   size = fread (read_start, 1, size, p_mp3->mp3_file);
   if (size <= 0)
      return -1;

   mad_stream_buffer (&p_mp3->mp3_stream, p_mp3->mp3_read_buf, size + remaining);
   p_mp3->mp3_stream.error = 0;

   return 0;
}

static gint mp3_decode_frame (private_mp3_s *p_mp3)
{
   if (p_mp3->mp3_stream.buffer == NULL ||
       p_mp3->mp3_stream.error == MAD_ERROR_BUFLEN) {
      if (mp3_fill_input (p_mp3) < 0) {
	 return DECODE_BREAK;
      }
   }

   if (mad_frame_decode (&p_mp3->mp3_frame, &p_mp3->mp3_stream)) {
      if (MAD_RECOVERABLE (p_mp3->mp3_stream.error)) {
	 return DECODE_CONT;
      } else {
	 if (p_mp3->mp3_stream.error == MAD_ERROR_BUFLEN) {
	    return DECODE_CONT;
	 } else {
	    LOG ("Unrecoverable frame error: '%s'",
		 mad_stream_errorstr (&p_mp3->mp3_stream));
	    p_mp3->mp3_status = 1;
	    return DECODE_BREAK;
	 }
      }
   }

   p_mp3->mp3_frame_count++;

   return DECODE_OK;
}

static gint mp3_read (song_s *song, gchar *buffer, gint *size)
{
   private_mp3_s *p_mp3 = (private_mp3_s *)song->private;
   static gint i;
   static gint ret;
   static struct audio_dither dither;
   guchar *out_ptr = buffer;

   mad_timer_add (&p_mp3->mp3_timer, p_mp3->mp3_frame.header.duration);
   mad_synth_frame (&p_mp3->mp3_synth, &p_mp3->mp3_frame);
   p_mp3->mp3_elapsed_time = ((float)mad_timer_count (p_mp3->mp3_timer,
						      MAD_UNITS_MILLISECONDS))/1000.0;
  
   for (i = 0; i < p_mp3->mp3_synth.pcm.length; i++) {
      gint sample;
      sample = (gint)audio_linear_dither (16, p_mp3->mp3_synth.pcm.samples[0][i], &dither);
   
      *(out_ptr++) = sample & 0xff;
      *(out_ptr++) = sample >> 8;

      if (MAD_NCHANNELS (&(p_mp3->mp3_frame).header) == 2) {
	 sample = (gint) audio_linear_dither (16, p_mp3->mp3_synth.pcm.samples[1][i], &dither);

	 *(out_ptr++) = sample & 0xff;
	 *(out_ptr++) = sample >> 8;
      }
   }

   while ((ret = mp3_decode_frame (p_mp3)) == DECODE_CONT)
      ;

   *size = p_mp3->mp3_synth.pcm.length * 4;
   return ret;
}

gchar *_input_play_chunk (song_s *song, gint *size, gchar *buf)
{
   private_mp3_s *p_mp3 = (private_mp3_s *)song->private;
   gchar *buffer = (gchar *)g_malloc (BUF_SIZE);
   if (buffer == NULL) {
      LOG ("BOOO a NULL malloc!");
      return NULL;
   }
   gint buf_size;

   if (mp3_read (song, buffer, &buf_size) == DECODE_BREAK) {
      song->finished = 1;
      g_free (buffer);
      return NULL;
   }
   //output_plugin_write_chunk_all (NULL, buffer, buf_size);
   *size = buf_size;
   
   return buffer;
}

song_s *_input_open (input_plugin_s *plugin, gchar *filename)
{
   struct stat filestat;
   int ret;
   private_mp3_s *p_mp3 = (private_mp3_s *)g_malloc (sizeof (private_mp3_s));
   memset (p_mp3, 0, sizeof (private_mp3_s));
   song_s *song = song_new (plugin, p_mp3);
   
   //p_mp3->mp3_total_time = 0.0;
   //p_mp3->mp3_elapsed_time = 0.0;
   //p_mp3->mp3_frame_count = 0;
   //p_mp3->mp3_status = 0;
   //p_mp3->mp3_start = 0;
   
   mad_stream_init (&p_mp3->mp3_stream);
   mad_frame_init (&p_mp3->mp3_frame);
   mad_timer_reset (&p_mp3->mp3_timer);

   if ((p_mp3->mp3_file = fopen (filename, "r")) <= 0) {
      LOG ("Problem opening file '%s'", filename);
      song_free (song);
      return NULL;
   }
   fstat (fileno (p_mp3->mp3_file), &filestat);
   while ((ret = mp3_decode_frame (p_mp3)) == DECODE_CONT)
      ; // do nothing 

   p_mp3->mp3_total_time = (filestat.st_size * 8.0) /
      p_mp3->mp3_frame.header.bitrate;

   LOG ("mp3 file '%s' opened", filename);

   return song;
}

gint _input_close (song_s *song)
{
   private_mp3_s *p_mp3 = (private_mp3_s *)song->private;
   mad_synth_finish (&p_mp3->mp3_synth);
   mad_frame_finish (&p_mp3->mp3_frame);
   mad_stream_finish (&p_mp3->mp3_stream);
   
   fclose (p_mp3->mp3_file);

   song_free (song);
   
   return 0;
}

gchar *_input_name (void)
{
   return plugin_name;
}

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

 * flac
 
*/

#define INPUT_IMPLEMENTATION
#import "input_plugin.h"
#import "bossao.h"
#import "thbuf.h"

#import <FLAC/file_decoder.h>
#import <FLAC/metadata.h>
#import <config.h>

static char *plugin_name="flac";

typedef struct private_flac_t {
   gdouble time_total;
   gdouble time_current;
   gint64 samples_total;
   gint64 samples_current;
   FLAC__FileDecoder *decoder;
   thbuf_sem_t *sem;
   gshort *buffer;
   gint buffer_size;
   gint was_metadata;
   gint64 total_samples;
} private_flac_s;

gint _input_identify (gchar *filename)
{
   int ret = 0;

   if (filename == NULL)
      return 0;

   int len = strlen (filename);
   gchar *ptr = &filename[len - 4];

   if (strncmp (ptr, "flac", 4) == 0)
      ret = 1;
   
   return ret;
}

gint _input_seek (song_s *song, gdouble length)
{
   LOG ("unimplemented");
   return 0;
}

gdouble _input_time_total (song_s *song)
{
   private_flac_s *p_flac = (private_flac_s *)song->private;
   return p_flac->time_total;
}

gdouble _input_time_current (song_s *song)
{
   private_flac_s *p_flac = (private_flac_s *)song->private;
   return p_flac->time_current;
}

gchar *_input_play_chunk (song_s *song, gint *size, gint64 *sample_num, gchar *eof)
{
   private_flac_s *p_flac = (private_flac_s *)song->private;
   FLAC__FileDecoder *decoder = (FLAC__FileDecoder *)p_flac->decoder;
   p_flac->was_metadata = 1;
   FLAC__file_decoder_process_single (decoder);

   if (FLAC__file_decoder_get_state (decoder) == FLAC__FILE_DECODER_END_OF_FILE) {
      //song->finished = 1;
      *sample_num = p_flac->samples_total;
      *size = 0;
      *eof = 1;
      LOG ("song is finished 1");
      return NULL;
   } else
      *eof = 0;

   *size = p_flac->buffer_size;
   *sample_num = p_flac->samples_current;

   if (p_flac->samples_current != p_flac->samples_total)
      semaphore_p (p_flac->sem);
   else {
      LOG ("setting eof early?");
      *eof = 1;
   }
   if (*size == 0) {
      //song->finished = 1;
      LOG ("song is finished 2");
      //return NULL;
   }
   return (gchar *)p_flac->buffer;
}

static void error_callback (const FLAC__FileDecoder *decoder,
			    const FLAC__StreamDecoderErrorStatus status,
			    void *data)
{
   LOG ("A FLAC error has occured");
}

static void metadata_callback (const FLAC__FileDecoder *decoder,
			       const FLAC__StreamMetadata *metadata,
			       void *data)
{
   song_s *song = (song_s *)data;
   private_flac_s *p_flac = (private_flac_s *)song->private;
   p_flac->was_metadata = 1;
   semaphore_v (p_flac->sem);
}

static FLAC__StreamDecoderWriteStatus write_callback (const FLAC__FileDecoder *decoder,
						      const FLAC__Frame *frame,
						      const FLAC__int32 *const buffer[],
						      void *data)
{
   song_s *song = (song_s *)data;
   private_flac_s *p_flac = (private_flac_s *)song->private;

   //gint size = frame->header.blocksize * frame->header.channels;
   gint samples = frame->header.blocksize;
   if (samples) {
      p_flac->buffer_size = samples * frame->header.channels * sizeof (gshort);
      p_flac->buffer = g_malloc (sizeof (gshort) * samples * frame->header.channels);
   } else {
      p_flac->buffer_size = 0;
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
   }
   gint c_samp, c_chan, d_samp;
   gshort *buf = p_flac->buffer;
   
   for (c_samp = d_samp = 0; c_samp < frame->header.blocksize; c_samp++) {
      for (c_chan = 0; c_chan < frame->header.channels; c_chan++, d_samp++) {
	 buf[d_samp] = buffer[c_chan][c_samp];
      }
   }
#ifdef WORDS_BIGENDIAN
  gchar *buf_pos = buf; 
  gchar *buf_end = buf + sizeof (buf);
  while (buf_pos < buf_end) {
    gchar p = *buf_pos;
    *buf_pos = *(buf_pos + 1);
    *(buf_pos + 1) = p;
    buf_pos += 2;
  }
#endif

  p_flac->samples_current += samples;
  p_flac->time_current += ((gdouble)samples) / frame->header.sample_rate;
  p_flac->was_metadata = 0;
  
  if (FLAC__file_decoder_get_state (decoder) == FLAC__FILE_DECODER_END_OF_FILE) {
     //song->finished = 1;
     LOG ("song is finished 3");
     //semaphore_v (p_flac->sem);
  }
  if (p_flac->samples_current  >= p_flac->samples_total) {
     LOG ("song is finished 4");
     //song->finished = 1;
     //semaphore_v (p_flac->sem);
  }
  
  semaphore_v (p_flac->sem);
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

song_s *_input_open (input_plugin_s *plugin, gchar *filename)
{
   private_flac_s *p_flac = (private_flac_s *)g_malloc (sizeof (private_flac_s));
   p_flac->decoder = FLAC__file_decoder_new ();
   p_flac->sem = semaphore_new (0);
   FLAC__FileDecoder *decoder = p_flac->decoder;
   song_s *song = song_new (plugin, p_flac);

   FLAC__file_decoder_set_metadata_ignore (decoder, FLAC__METADATA_TYPE_STREAMINFO);
   
   FLAC__file_decoder_set_write_callback (decoder, write_callback);
   FLAC__file_decoder_set_metadata_callback (decoder, metadata_callback);
   FLAC__file_decoder_set_error_callback (decoder, error_callback);
   FLAC__file_decoder_set_client_data (decoder, song);

   FLAC__file_decoder_set_filename (decoder, filename);

   FLAC__SeekableStreamDecoderState state = FLAC__file_decoder_init (decoder);
   if (state != FLAC__FILE_DECODER_OK) {
      printf ("Problem initializing FLAC file decoder: ");
      if (state == FLAC__FILE_DECODER_ALREADY_INITIALIZED)
	 printf ("already inited\n");
      else if (state == FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR)
	 printf ("seekable decoder error\n");
      else
	 printf ("unknown error: %d\n", state);
      FLAC__file_decoder_delete (decoder);
      song_free (song);
      g_free (p_flac);
      return NULL;
   }

   FLAC__Metadata_SimpleIterator *it = FLAC__metadata_simple_iterator_new ();
   if (!FLAC__metadata_simple_iterator_init (it, filename, 1, 0)) {
      FLAC__metadata_simple_iterator_delete (it);
      LOG ("Problem iterating over metadata");
      FLAC__file_decoder_delete (decoder);
      song_free (song);
      g_free (p_flac);
      return NULL;
   }

   FLAC__StreamMetadata *block = NULL;
   do {
      if (block)
	 FLAC__metadata_object_delete (block);
      block = FLAC__metadata_simple_iterator_get_block (it);
      if (block->type == FLAC__METADATA_TYPE_STREAMINFO)
	 break;
   } while (FLAC__metadata_simple_iterator_next (it));

   p_flac->total_samples = block->data.stream_info.total_samples;
   p_flac->time_total = ((gdouble)block->data.stream_info.total_samples) /
      block->data.stream_info.sample_rate;
   p_flac->time_current = 0;
   p_flac->samples_total = block->data.stream_info.total_samples;
   p_flac->samples_current = 0;

   FLAC__metadata_object_delete (block);
   FLAC__metadata_simple_iterator_delete (it);

   // preload some decodes
   FLAC__file_decoder_process_single (decoder);
   FLAC__file_decoder_process_single (decoder);
   FLAC__file_decoder_process_single (decoder);
   FLAC__file_decoder_process_single (decoder);

   return song;
}

gint64 _input_samples_total (song_s *song)
{
   private_flac_s *p_flac = (private_flac_s *)song->private;
   return p_flac->total_samples;
}

gint _input_close (song_s *song)
{
   private_flac_s *p_flac = (private_flac_s *)song->private;
   FLAC__FileDecoder *decoder = (FLAC__FileDecoder *)p_flac->decoder;

   if (decoder != NULL) {
      FLAC__file_decoder_finish (decoder);
      FLAC__file_decoder_delete (decoder);
      decoder= NULL;
   }

   song_free (song);
   g_free (p_flac);
   
   return 0;
}

gchar *_input_name (void)
{
   return plugin_name;
}

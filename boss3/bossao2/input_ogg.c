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

 * input plugin for ogg vorbis files
 
*/

#define INPUT_IMPLEMENTATION
#import "input_plugin.h"
#import "bossao.h"
#import <vorbis/vorbisfile.h>

static gchar *plugin_name = "ogg";
//static OggVorbis_File vorbis_file;

typedef struct private_ogg_t {
   gchar *plugin_name;
   OggVorbis_File vorbis_file;
} private_ogg_s;

/* return 1 if the plugin handles this file
   currently, this only looks at the file extension
*/
gint _input_identify (gchar *filename)
{
   int ret = 0;

   if (filename == NULL)
      return 0;

   gint len = strlen (filename);
   gchar *ptr = &filename[len - 3];

   if (strncmp (ptr, "ogg", 3) == 0)
      ret = 1;
   
   return ret;
}

/* seek a given distance */
gint _input_seek (song_s *song, gdouble length)
{
   private_ogg_s *p_ogg = (private_ogg_s *)song->private;
   ov_time_seek (&p_ogg->vorbis_file, length);
   return 0;
}

/* return the total time of the file */
gdouble _input_time_total (song_s *song)
{
   private_ogg_s *p_ogg = (private_ogg_s *)song->private;
   return ov_time_total (&p_ogg->vorbis_file, -1);
}

gint64 _input_samples_total (song_s *song)
{
   private_ogg_s *p_ogg = (private_ogg_s *)song->private;
   return ov_pcm_total (&p_ogg->vorbis_file, -1);
}

/* return the current time
   currently broken since the read thread will be ahead of the write thread
*/
gdouble _input_time_current (song_s *song)
{
   //LOG ("TODO: fix current time with threaded buffer");
   private_ogg_s *p_ogg = (private_ogg_s *)song->private;
   return ov_time_tell (&p_ogg->vorbis_file);
}

/* play a chunk */
chunk_s *_input_play_chunk (song_s *song, gint *size, gint64 *sample_num, gchar *eof)
{
   gint current;
   chunk_s *chunk = (chunk_s *)g_malloc (sizeof (chunk_s));
   chunk->chunk = (gchar *)g_malloc (BUF_SIZE);
   //gchar *ret = g_malloc (BUF_SIZE);
   private_ogg_s *p_ogg = (private_ogg_s *)song->private;
   //sample_num = ov_pcm_tell (&p_ogg->vorbis_file);
   chunk->sample_num = ov_pcm_tell (&p_ogg->vorbis_file);
   //*size = ov_read (&p_ogg->vorbis_file, ret, BUF_SIZE / 2, 0, 2, 1, &current);
   chunk->size = ov_read (&p_ogg->vorbis_file, chunk->chunk, BUF_SIZE / 2, 0, 2, 1, &current);
   
   if (chunk->size == OV_EBADLINK || chunk->size == OV_HOLE) {
      chunk->size = 0;
      g_free (chunk->chunk);
      chunk->chunk = NULL;
      return chunk;
   }
   
/*
   if (*size == OV_EBADLINK || *size == OV_HOLE)
      *size = 0;
      
   if (*size < 1) {
      *sample_num = ov_pcm_tell (&p_ogg->vorbis_file);
      *eof = 1;
      */
   if (chunk->size < 1) {
      chunk->sample_num = ov_pcm_tell (&p_ogg->vorbis_file);
      chunk->eof = 1;
      //song->finished = 1;
/*
      song->finished = 1;
      LOG ("end of file?");
      */
      //g_free (ret);
      g_free (chunk->chunk);
      g_free (chunk);
      chunk->chunk = NULL;
      chunk->size = 0;
      return chunk;
   } else {
      //*eof = 0;
      chunk->eof = 0;
   }
   
   return chunk;
}

/* open the file, perform a couple checks */
song_s *_input_open (input_plugin_s *plugin, gchar *filename)
{
   vorbis_info *vi = NULL;
   FILE *file;
   private_ogg_s *p_ogg = (private_ogg_s *)g_malloc (sizeof (private_ogg_s));
   p_ogg->plugin_name = plugin_name;
   song_s *song = song_new (plugin, p_ogg);

   /* open the file on disk */
   file = fopen (filename, "r");
   if (file == NULL) {
      LOG ("Error opening '%s'", filename);
      song_free (song);
      return NULL;
   }
   /* connect it to the ogg system */
   if (ov_open (file, &p_ogg->vorbis_file, NULL, 0) < 0) {
      LOG ("File '%s' does not appear to be an ogg vorbis file", filename);
      ov_clear (&p_ogg->vorbis_file);
      song_free (song);
      return NULL;
   }

   vi = ov_info (&p_ogg->vorbis_file, -1);
   /* check the number of channels */
   if (vi->channels != 2) {
      LOG ("%d channels is not currently supported", vi->channels);
      ov_clear (&p_ogg->vorbis_file);
      song_free (song);
      return NULL;
   }
   /* check the rate */
   if (vi->rate != 44100) {
      LOG ("A sample rate of %ld is not currently supported", vi->rate);
      ov_clear (&p_ogg->vorbis_file);
      song_free (song);
      return NULL;
   }

   return song;
}

/* close the file */
gint _input_close (song_s *song)
{
   private_ogg_s *p_ogg = (private_ogg_s *)song->private;
   ov_clear (&p_ogg->vorbis_file);
   LOG ("TODO: is this leaking files?");

   song_free (song);
   //g_free (p_ogg);
   
   return 0;
}

/* return the name of this plugin */
gchar *_input_name (void)
{
   return plugin_name;
}

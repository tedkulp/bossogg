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
#import <vorbis/vorbisfile.h>

static gchar *plugin_name = "ogg";
static OggVorbis_File vorbis_file;

/* return 1 if the plugin handles this file
   currently, this only looks at the file extension
*/
gint input_identify (gchar *filename)
{
   int ret = 0;

   if (filename == NULL)
      return 0;

   int len = strlen (filename);
   gchar *ptr = &filename[len - 3];

   if (strncmp (ptr, "ogg", 3) == 0)
      ret = 1;
   
   return ret;
}

/* seek a given distance */
gint input_seek (song_s *song, gdouble length)
{
   ov_time_seek (&vorbis_file, length);
   return 0;
}

/* return the total time of the file */
gdouble input_time_total (song_s *song)
{
   return ov_time_total (&vorbis_file, -1);
}

/* return the current time
   currently broken since the read thread will be ahead of the write thread
*/
gdouble input_time_current (song_s *song)
{
   LOG ("TODO: fix current time with threaded buffer");
   return ov_time_tell (&vorbis_file);
}

/* play a chunk */
gchar *input_play_chunk (song_s *song, gint *size, gchar *buf)
{
   gint current; 
   gchar *ret = g_malloc (BUF_SIZE);
   *size = ov_read (&vorbis_file, ret, BUF_SIZE / 2, 0, 2, 1, &current);
   
   return ret;
}

/* open the file, perform a couple checks */
gint input_open (song_s *song, gchar *filename)
{
   vorbis_info *vi = NULL;
   FILE *file;

   /* open the file on disk */
   file = fopen (filename, "r");
   if (file == NULL) {
      LOG ("Error opening '%s'", filename);
      return -1;
   }
   /* connect it to the ogg system */
   if (ov_open (file, &vorbis_file, NULL, 0) < 0) {
      LOG ("File '%s' does not appear to be an ogg vorbis file", filename);
      ov_clear (&vorbis_file);
      return -2;
   }

   vi = ov_info (&vorbis_file, -1);
   /* check the number of channels */
   if (vi->channels != 2) {
      LOG ("%d channels is not currently supported", vi->channels);
      ov_clear (&vorbis_file);
      return -3;
   }
   /* check the rate */
   if (vi->rate != 44100) {
      LOG ("A sample rate of %ld is not currently supported", vi->rate);
      ov_clear (&vorbis_file);
      return -4;
   }
      
   return 0;
}

/* close the file */
gint input_close (song_s *song)
{
   ov_clear (&vorbis_file);
   LOG ("TODO: is this leaking files?");
   
   return 0;
}

/* return the name of this plugin */
gchar *input_name (void)
{
   return plugin_name;
}

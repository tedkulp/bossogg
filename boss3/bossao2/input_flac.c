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

static char *plugin_name="flac";

gint input_identify (gchar *filename)
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

gint input_seek (song_s *song, gdouble length)
{
   return 0;
}

gdouble input_time_total (song_s *song)
{
   return 0;
}

gdouble input_time_current (song_s *song)
{
   return 0;
}

gchar *input_play_chunk (song_s *song, gint *size, gchar *buf)
{
   return NULL;
}

gint input_open (song_s *song, gchar *filename)
{
   return 0;
}

gint input_close (song_s *song)
{
   return 0;
}

gchar *input_name (void)
{
   return plugin_name;
}

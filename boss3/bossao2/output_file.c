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

 * file output plugin
 
*/

#import "output_plugin.h"

static gchar *plugin_name = "file";
static FILE *file = NULL;
static gchar *filename = NULL;

gint output_open (PyObject *cfgparser)
{
   LOG ("TODO: get the output filename from cfgparser");
   filename = NULL;
   file = fopen (NULL, "w");
   if (file == NULL) {
      printf ("Problem opening file '%s' for writing", "somefilename");
      return -1;
   }
   
   return 0;
}

void output_close (void)
{
   if (file == NULL) {
      LOG ("Tried to close a NULL file");
      return;
   }
   fclose (file);
}

gint output_write_chunk (gchar *buffer, gint size)
{
   fwrite (buffer, size, 1, file);
   return 0;
}

gchar *output_name (void)
{
   return plugin_name;
}

gchar *output_driver_name (void)
{
   return plugin_name;
}

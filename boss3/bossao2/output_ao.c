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

 * libao output plugin
 
*/

#import "output_plugin.h"
#import <ao/ao.h>

static gchar *plugin_name = "libao";
static ao_device *device = NULL;
static gint driver_id;

gint output_open (PyObject *cfgparser)
{
   ao_sample_format format;

   format.bits = 16;
   format.rate = 44100;
   format.channels = 2;
   format.byte_format = AO_FMT_LITTLE;

   ao_initialize ();

   driver_id = ao_default_driver_id ();
   device = ao_open_live (driver_id, &format, NULL);
   if (device == NULL) {
      LOG ("Problem opening audio output device");
      return -1;
   }
   
   return 0;
}

void output_close (void)
{
   ao_close (device);
   ao_shutdown ();
}

gint output_write_chunk (guchar *buffer, gint size)
{
   //LOG ("writing %p %d", buffer, size);
   ao_play (device, buffer, size);
   
   return 0;
}

gchar *output_name (void)
{
   return plugin_name;
}

gchar *output_driver_name (void)
{
   ao_info *info;

   if (device == NULL) {
      LOG ("Tried to get the driver name of a NULL device");
      return NULL;
   }

   info = ao_driver_info (driver_id);
   return info->short_name;
}

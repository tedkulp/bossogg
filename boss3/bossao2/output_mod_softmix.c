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

 * software mixer output mod plugin
 
*/

#import "output_mod_plugin.h"

static gchar *plugin_name = "softmix";
static gchar *description = "Software Mixing";

static int volume = 100;
static gdouble percent = 1.0;

gchar *output_mod_name (void)
{
   return plugin_name;
}

gchar *output_mod_description (void)
{
   return description;
}

void output_mod_configure (gint arg1, gint arg2, gpointer user_data)
{
   if (user_data != NULL) {
      volume = *(gint *)user_data;
   }
   // make sure it is a sane value
   if (volume > 100)
      volume = 100;
   if (volume < 0)
      volume = 0;
   percent = (gdouble)volume / 100.0;
}

void output_mod_get_config (gint *arg1, gint *arg2, gpointer *user_data)
{
   if (user_data != NULL)
      *user_data = &volume;
   if (arg1 != NULL)
      *arg1 = 0;
   if (arg2 != NULL)
      *arg2 = 0;
}

void output_mod_run (gchar *chunk, gint size)
{
   gint i;

   if (chunk == NULL || percent == 1.0)
      return;
   if (percent == 0.0) {
      memset (chunk, 0, size);
      return;
   }
   
   // software mixing is easy, all you have to do is multiply
   // the samples by the volume percentage you want
   gshort *p = (gshort *)chunk;
   for (i = 0; i < size / 2; i++) {
      *p++ *= percent;
   }
}

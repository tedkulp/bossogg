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
   
}

void output_mod_run (gchar *chunk, gint size)
{
   gint i;

   // software mixing is easy, all you have to do is multiply
   // the samples by the volume percentage you want
   gshort *p = chunk;
   for (i = 0; i < size / 2; i++) {
      //*p++ *= 0.2;
   }
}

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

 * This header shows the interface for output plugins
 
*/

#import "common.h"
#import "bossao.h"

typedef gint (*output_open_f)(PyObject *cfgparser);
typedef void (*output_close_f)(void);
typedef gint (*output_write_chunk_f)(guchar *buffer, gint size);
typedef gchar *(*output_name_f)(void);
typedef gchar *(*output_driver_name_f)(void);

typedef struct output_plugin_t {
   output_open_f output_open;
   output_close_f output_close;
   output_write_chunk_f output_write_chunk;
   output_name_f output_name;
   output_driver_name_f output_driver_name;
   GModule *lib;
   gchar *name;
} output_plugin_s;

void output_plugin_clear (output_plugin_s *plugin);
void output_plugin_set (output_plugin_s *plugin);
output_plugin_s *output_plugin_open (gchar *filename);
void output_plugin_close (output_plugin_s *plugin);
output_plugin_s *output_plugin_find (gchar *filename);
void output_plugin_open_all (PyObject *cfgparser);
void output_plugin_close_all (void);
void output_plugin_write_chunk_all (guchar *buffer, gint size);

gint output_open (PyObject *cfgparser);
void output_close (void);
gint output_write_chunk (guchar *buffer, gint size);
gchar *output_name (void);
gchar *output_driver_name (void);

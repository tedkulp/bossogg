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

 *********************************************************************
 * output plugin implementation
 *
 * the output plugins are meant to be used as a whole
 * this is because when a song is playing, it needs to be output to the
 * audio device in real time, possibly re-encoded for shout, etc all at
 * the same time
 *********************************************************************
 
*/

#import "output_plugin.h"

static GSList *output_list = NULL;

/* set the function pointers of an output plugin to NULL */
void output_plugin_clear (output_plugin_s *plugin)
{
   plugin->output_open = NULL;
   plugin->output_close = NULL;
   plugin->output_write_chunk = NULL;
}

/* attempt to open the output plugin */
output_plugin_s *output_plugin_open (gchar *filename)
{
   GModule *lib = g_module_open (filename, G_MODULE_BIND_LAZY);
   if (lib == NULL) {
      LOG ("Could not dlopen '%s': %s", filename, g_module_error ());
      return NULL;
   }

   output_plugin_s *plugin = (output_plugin_s *)g_malloc (sizeof (output_plugin_s));

   plugin->output_open = (output_open_f)get_symbol (lib, "output_open");
   plugin->output_close = (output_close_f)get_symbol (lib, "output_close");
   plugin->output_write_chunk = (output_write_chunk_f)get_symbol (lib, "output_write_chunk");
   plugin->output_name = (output_name_f)get_symbol (lib, "output_name");
   plugin->output_driver_name = (output_driver_name_f)get_symbol (lib, "output_driver_name");
   plugin->lib = lib;
   plugin->name = plugin->output_name ();

   output_list = g_slist_append (output_list, plugin);

   LOG ("Output plugin '%s', for %s output loaded", filename, plugin->name);

   return plugin;
}

/* close (and free) an open output plugin */
void output_plugin_close (output_plugin_s *plugin)
{
   output_list = g_slist_remove (output_list, plugin);
   LOG ("Closing module '%s'", plugin->name);
   g_module_close (plugin->lib);
   g_free (plugin);
}

static void output_plugin_open_all_helper (gpointer item, gpointer user_data)
{
   output_plugin_s *plugin = (output_plugin_s *)item;
   PyObject *cfgparser = (PyObject *)user_data;
   
   plugin->output_open (cfgparser);
}

/* open all output plugins */
void output_plugin_open_all (PyObject *cfgparser)
{
   if (output_list == NULL) {
      LOG ("Output plugin list is uninitialized");
      return;
   }

   g_slist_foreach (output_list, output_plugin_open_all_helper, cfgparser);

   return;
}

static void output_plugin_close_all_helper (gpointer item, gpointer user_data)
{
   output_plugin_s *plugin = (output_plugin_s *)item;
   LOG ("Closing module '%s'", plugin->name);
   g_module_close (plugin->lib);
   g_free (plugin);
}

/* close all output plugins */
void output_plugin_close_all (void)
{
   if (output_list == NULL) {
      LOG ("Output plugin list is already empty");
      return;
   }

   g_slist_foreach (output_list, output_plugin_close_all_helper, NULL);
}

typedef struct write_chunk_all_t {
   gchar *buffer;
   gint size;
} write_chunk_all_s;

static void output_plugin_write_chunk_all_helper (gpointer item, gpointer user_data)
{
   output_plugin_s *plugin = (output_plugin_s *)item;
   write_chunk_all_s *s = (write_chunk_all_s *)user_data;

   plugin->output_write_chunk (s->buffer, s->size);
}

/* write the buffer to all output plugins */
void output_plugin_write_chunk_all (gchar *buffer, gint size)
{
   write_chunk_all_s s;
   
   if (output_list == NULL) {
      LOG ("Output plugin list is uninitialized");
      return;
   }

   s.buffer = buffer;
   s.size = size;
   if (!buffer || size <= 0)
      return;
   g_slist_foreach (output_list, output_plugin_write_chunk_all_helper, &s);
   
   return;
}

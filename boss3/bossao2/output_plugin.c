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

GSList *output_list = NULL;

/* set the function pointers of an output plugin to NULL */
void output_plugin_clear (output_plugin_s *plugin)
{
   plugin->output_open = NULL;
   plugin->output_close = NULL;
   plugin->output_write_chunk = NULL;
}

/* some error checking around g_module_symbol */
static gpointer get_symbol (GModule *lib, char *name)
{
   gpointer symbol;
   gboolean ret;
   ret = g_module_symbol (lib, name, &symbol);
   if (symbol == NULL) {
      LOG ("Couldn't find symbol '%s' in library", name);
   }
   return symbol;
}

/* attempt to open the output plugin */
output_plugin_s *output_plugin_open (gchar *filename)
{
   GModule *lib = g_module_open (filename, G_MODULE_BIND_MASK);
   if (lib == NULL) {
      LOG ("Could not dlopen '%s': %s", filename, g_module_error ());
      return NULL;
   }

   output_plugin_s *plugin = (output_plugin_s *)g_malloc (sizeof (output_plugin_s));

   plugin->output_open = get_symbol (lib, "output_open");
   plugin->output_close = get_symbol (lib, "output_close");
   plugin->output_write_chunk = get_symbol (lib, "output_write_chunk");
   plugin->output_name = get_symbol (lib, "output_name");
   plugin->output_driver_name = get_symbol (lib, "output_driver_name");
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

   g_module_close (plugin->lib);
   g_free (plugin);
}

/* open all output plugins */
void output_plugin_open_all (PyObject *cfgparser)
{
   output_plugin_s *plugin;
   GSList *list = output_list;

   if (list == NULL) {
      LOG ("Output plugin list is uninitialized");
      return;
   }

   do {
      plugin = (output_plugin_s *)list->data;
      plugin->output_open (cfgparser);
   } while ((list = g_slist_next (list)) != NULL);

   return;
}

/* close all output plugins */
void output_plugin_close_all (void)
{
   output_plugin_s *plugin;

   if (output_list == NULL) {
      LOG ("Output plugin list is already empty");
      return;
   }

   do {
      plugin = (output_plugin_s *)output_list->data;
      LOG ("Closing module '%s'", plugin->name);
      g_module_close (plugin->lib);
      g_free (plugin);
   } while ((output_list = g_slist_remove (output_list, output_list->data)) != NULL);
}

/* write the buffer to all output plugins */
void output_plugin_write_chunk_all (song_s *song, unsigned char *buffer, gint size)
{
   GSList *list = output_list;
   output_plugin_s *plugin;

   if (list == NULL) {
      LOG ("Output plugin list is uninitialized");
      return;
   }
   do {
      plugin = (output_plugin_s *)list->data;
      plugin->output_write_chunk (song, buffer, size);
   } while ((list = g_slist_next (list)) != NULL);

   return;
}

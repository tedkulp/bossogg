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

 *************************************************************************
 * input plugin implementation
 *
 * the input plugin is currently limited to using one plugin at a time
 * ...since the the read/write cycles are now threaded hopefully this won't
 * be a problem
 *************************************************************************
 
*/

#define INPUT_IMPLEMENTATION
#import "input_plugin.h"

input_plugin_s *current_plugin = NULL;

GSList *input_list = NULL;

inline gint input_identify (gchar *filename)
{
   return current_plugin->input_identify (filename);
}

inline gint input_seek (song_s *song, gdouble len)
{
   return current_plugin->input_seek (song, len);
}

inline gdouble input_time_total (song_s *song)
{
   return current_plugin->input_time_total (song);
}

inline gdouble input_time_current (song_s *song)
{
   return current_plugin->input_time_current (song);
}

inline gchar *input_play_chunk (song_s *song, gint *size, gchar *buf)
{
   return current_plugin->input_play_chunk (song, size, buf);
}

inline gint input_open (song_s *song, gchar *filename)
{
   return current_plugin->input_open (song, filename);
}

inline gint input_close (song_s *song)
{
   return current_plugin->input_close (song);
}

inline gchar *input_name (void)
{
   return current_plugin->input_name ();
}

/* if plugin is NULL, clear the currently used plugin
   else, clear the given plugin */
void input_plugin_clear (input_plugin_s *plugin)
{
   if (plugin != NULL) {
      plugin->input_identify = NULL;
      plugin->input_seek = NULL;
      plugin->input_time_total = NULL;
      plugin->input_time_current = NULL;
      plugin->input_play_chunk = NULL;
      plugin->input_open = NULL;
      plugin->input_close = NULL;
      plugin->input_name = NULL;
   } else {
      current_plugin = NULL;
   }
}

/* set the currently used plugin to the given plugin */
void input_plugin_set (input_plugin_s *plugin)
{
   current_plugin = plugin;
}

/* some error checking around g_module_symbol */
static gpointer get_symbol (GModule *lib, gchar *name)
{
   gpointer symbol;
   gboolean ret;
   ret = g_module_symbol (lib, name, &symbol);
   if (symbol == NULL) {
      LOG ("Couldn't find symbol '%s' in library", name);
   }
   return symbol;
}

/* attempt to open the input plugin  */
input_plugin_s *input_plugin_open (gchar *filename)
{
   gint module_bind = -1;
   if (!GLIB_CHECK_VERSION (2, 4, 0)) {
      printf ("glib < 2.4 detected, using lazy dynamic loading (may be slow)\n");
      module_bind = G_MODULE_BIND_LAZY;
   } else
      module_bind = G_MODULE_BIND_LOCAL;
   GModule *lib = g_module_open (filename, module_bind);
   if (lib == NULL) {
      LOG ("Could not dlopen '%s': %s", filename, g_module_error ());
      return NULL;
   }

   input_plugin_s *plugin = (input_plugin_s *)g_malloc (sizeof (input_plugin_s));

   plugin->input_identify = (input_identify_f)get_symbol (lib, "input_identify");
   plugin->input_seek = (input_seek_f)get_symbol (lib, "input_seek");
   plugin->input_time_total = (input_time_total_f)get_symbol (lib, "input_time_total");
   plugin->input_time_current = (input_time_current_f)get_symbol (lib, "input_time_current");
   plugin->input_play_chunk = (input_play_chunk_f)get_symbol (lib, "input_play_chunk");
   plugin->input_open = (input_open_f)get_symbol (lib, "input_open");
   plugin->input_close = (input_close_f)get_symbol (lib, "input_close");
   plugin->input_name = (input_name_f)get_symbol (lib, "input_name");
   plugin->lib = lib;
   plugin->name = plugin->input_name ();

   input_list = g_slist_append (input_list, plugin);

   if (current_plugin == NULL)
      current_plugin = plugin;

   LOG ("Input plugin '%s', for '%s' files loaded", filename, plugin->name);
   
   return plugin;
}

/* close (and free) an open input plugin */
void input_plugin_close (input_plugin_s *plugin)
{
   input_list = g_slist_remove (input_list, plugin);
   
   g_module_close (plugin->lib);
   g_free (plugin);
}

/* close all input plugins */
void input_plugin_close_all (void)
{
   input_plugin_s *plugin;
   
   if (input_list == NULL) {
      LOG ("Input plugin list is already empty");
      return;
   }

   do {
      plugin = (input_plugin_s *)input_list->data;
      LOG ("Closing module '%s'", plugin->name);
      g_module_close (plugin->lib);
      g_free (plugin);
   } while ((input_list = g_slist_remove (input_list, input_list->data)) != NULL);
}

/* attempt to locate the input plugin for a given file */
input_plugin_s *input_plugin_find (char *filename)
{
   GSList *list = input_list;
   input_plugin_s *plugin;

   LOG ("Trying to find an input plugin for '%s'", filename);

   if (list == NULL) {
      LOG ("Input plugin list is uninitialized");
      return NULL;
   }
   do {
      plugin = (input_plugin_s *)list->data;
      if (plugin->input_identify (filename)) {
	 LOG ("Found a file of type '%s'", plugin->name);
	 return plugin;
      } else
	 LOG ("File is not %s", plugin->name);
   } while ((list = g_slist_next (list)) != NULL);

   return NULL;
}

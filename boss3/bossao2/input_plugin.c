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
#import "bossao.h"

static GMutex *current_mutex;
static input_plugin_s *current_plugin = NULL;
static song_s *current_song = NULL;

static GSList *input_list = NULL;

song_s *song_new (input_plugin_s *input_plugin, void *data)
{
   song_s *song = g_malloc (sizeof (song_s));
   song->input_plugin = input_plugin;
   song->private = data;
   song->filename = NULL;
   song->finished = 0;
   return song;
}

void song_free (song_s *song)
{
   g_free (song->private);
   g_free (song);
}

inline gint input_identify (gchar *filename)
{
   gint ret;
   g_mutex_lock (current_mutex);
   if (current_plugin != NULL) {
      ret = current_plugin->input_identify (filename);
   } else {
      LOG ("current plugin is NULL");
      ret = -1;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline gint input_seek (gdouble len)
{
   gint ret;
   g_mutex_lock (current_mutex);
   if (current_song != NULL) {
      ret = current_plugin->input_seek (current_song, len);
   } else {
      LOG ("current song is NULL");
      ret = -1;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline gdouble input_time_total (void)
{
   gdouble ret;
   g_mutex_lock (current_mutex);
   if (current_song != NULL) {
      ret = current_plugin->input_time_total (current_song);
   } else {
      LOG ("current song is NULL");
      ret = -1;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline gdouble input_time_current (void)
{
   gdouble ret;
   g_mutex_lock (current_mutex);
   if (current_song != NULL) {
      ret = current_plugin->input_time_current (current_song);
   } else {
      LOG ("current song is NULL");
      ret = -1;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline gchar *input_play_chunk (gint *size, gint64 *sample_num, gchar *eof)
{
   gchar *ret;
   g_mutex_lock (current_mutex);
   if (current_song != NULL && current_plugin != NULL) {
      //LOG ("playing a chunk of %s", current_plugin->name);
      //g_usleep (0);
      ret = current_plugin->input_play_chunk (current_song, size, sample_num, eof);
   } else {
      *eof = 0;
      *sample_num = 0; 
      LOG ("current song is NULL");
      ret = NULL;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline gint input_finished (void)
{
   gint ret;
   g_mutex_lock (current_mutex);
   if (current_song != NULL) {
      ret = current_song->finished;
   } else {
      LOG ("current song is NULL");
      ret = 0;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline song_s *input_open (input_plugin_s *plugin, gchar *filename)
{
   song_s *song = plugin->input_open (plugin, filename);
   song->filename = filename;
   g_mutex_lock (current_mutex);
   current_song = song;
   g_mutex_unlock (current_mutex);
   return song;
}

inline gint input_close (void)
{
   gint ret;
   g_mutex_lock (current_mutex);
   if (current_song != NULL) {
      if (current_plugin != NULL)
	 ret = current_plugin->input_close (current_song);
      else
	 LOG ("current plugin was NULL!");
      current_song = NULL;
   } else {
      LOG ("already NULL");
      ret = -1;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline gchar *input_name (void)
{
   gchar *ret;
   g_mutex_lock (current_mutex);
   if (current_plugin != NULL) {
      ret = current_plugin->input_name ();
   } else {
      LOG ("current plugin is NULL");
      ret = NULL;
   }
   g_mutex_unlock (current_mutex);
   return ret;
}

inline gchar *input_filename (void)
{
   gchar *ret;
   g_mutex_lock (current_mutex);
   if (current_plugin != NULL) {
      if (current_song != NULL)
	 ret = current_song->filename;
   } else {
      LOG ("current plugin is NULL");
      ret = NULL;
   }
   g_mutex_unlock (current_mutex);
   return ret;
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
      g_mutex_lock (current_mutex);
      current_plugin = NULL;
      g_mutex_unlock (current_mutex);
   }
}

/* set the currently used plugin to the given plugin */
void input_plugin_set (input_plugin_s *plugin)
{
   g_mutex_lock (current_mutex);
   current_plugin = plugin;
   current_song = NULL;
   g_mutex_unlock (current_mutex);
}

void input_plugin_set_end_of_file (void)
{
   if (current_song != NULL) {
      g_mutex_lock (current_mutex);
      current_song->finished = 1;
      g_mutex_unlock (current_mutex);
   } 
}

gint64 input_plugin_samples_total (void)
{
   gint64 ret;
   g_mutex_lock (current_mutex);
   if (current_plugin != NULL) {
      if (current_song != NULL) {
	 ret = current_plugin->input_samples_total (current_song);
      } else {
	 LOG ("current song is NULL");
	 ret = -1;
      }
   } else {
      LOG ("current plugin is NULL");
      ret = -1;
   }

   g_mutex_unlock (current_mutex);
   return ret;
}

/* attempt to open the input plugin  */
input_plugin_s *input_plugin_open (gchar *filename)
{
   GModule *lib = g_module_open (filename, G_MODULE_BIND_LAZY);
   if (lib == NULL) {
      LOG ("Could not dlopen '%s': %s", filename, g_module_error ());
      return NULL;
   }

   if (current_mutex == NULL) {
      LOG ("initializing current mutex");
      current_mutex = g_mutex_new ();
   }
   
   input_plugin_s *plugin = (input_plugin_s *)g_malloc (sizeof (input_plugin_s));

   plugin->input_identify = (input_identify_f)get_symbol (lib, "_input_identify");
   plugin->input_seek = (input_seek_f)get_symbol (lib, "_input_seek");
   plugin->input_samples_total = (input_samples_total_f)get_symbol (lib, "_input_samples_total");
   plugin->input_time_total = (input_time_total_f)get_symbol (lib, "_input_time_total");
   plugin->input_time_current = (input_time_current_f)get_symbol (lib, "_input_time_current");
   plugin->input_play_chunk = (input_play_chunk_f)get_symbol (lib, "_input_play_chunk");
   plugin->input_open = (input_open_f)get_symbol (lib, "_input_open");
   plugin->input_close = (input_close_f)get_symbol (lib, "_input_close");
   plugin->input_name = (input_name_f)get_symbol (lib, "_input_name");
   plugin->lib = lib;
   plugin->name = plugin->input_name ();

   input_list = g_slist_append (input_list, plugin);

   if (current_plugin == NULL) {
      g_mutex_lock (current_mutex);
      current_plugin = plugin;
      g_mutex_unlock (current_mutex);
   }

   LOG ("Input plugin '%s':%p, for '%s' files loaded", filename, plugin, plugin->name);
   
   return plugin;
}

/* close (and free) an open input plugin */
void input_plugin_close (input_plugin_s *plugin)
{
   input_list = g_slist_remove (input_list, plugin);

   g_mutex_lock (current_mutex);
   if (current_plugin == plugin)
      current_plugin = NULL;
   g_mutex_unlock (current_mutex);
   
   g_module_close (plugin->lib);
   g_free (plugin);
}

static void input_plugin_close_all_helper (gpointer item, gpointer user_data)
{
   input_plugin_s *plugin = (input_plugin_s *)item;
   if (plugin == NULL) {
      LOG ("NULL plugin??");
   }
   LOG ("Closing module '%s'", plugin->name);
   g_module_close (plugin->lib);
   g_free (plugin);
}

/* close all input plugins */
void input_plugin_close_all (void)
{
   if (input_list == NULL) {
      LOG ("Input plugin list is already empty");
      return;
   }

   g_mutex_lock (current_mutex);
   current_plugin = NULL;
   g_mutex_unlock (current_mutex);

   g_slist_foreach (input_list, input_plugin_close_all_helper, NULL);
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

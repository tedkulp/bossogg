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
 * output modification plugin implementation
 *
 * the output mod plugins are meant to be used as a
 * tweak on the audio output, before it is send to the sound card
 * for example, cross fading and software mixing could be performed
 * here
 *********************************************************************
 
*/

#import "output_mod_plugin.h"

static GSList *output_mod_list = NULL;

/* set the function pointers to NULL */
void output_mod_plugin_clear (output_mod_plugin_s *plugin)
{
   
}

/* try and open output mod plugin */
output_mod_plugin_s *output_mod_plugin_open (gchar *filename)
{
   GModule *lib = g_module_open (filename, G_MODULE_BIND_LAZY);
   if (lib == NULL) {
      LOG ("Could not dlopen '%s': %s", filename, g_module_error ());
      return NULL;
   }

   output_mod_plugin_s *plugin = (output_mod_plugin_s *)g_malloc (sizeof (output_mod_plugin_s));
   plugin->output_mod_description = (output_mod_description_f)get_symbol (lib,
									  "output_mod_description");
   plugin->output_mod_configure = (output_mod_configure_f)get_symbol (lib, "output_mod_configure");
   plugin->output_mod_run = (output_mod_run_f)get_symbol (lib, "output_mod_run");
   plugin->output_mod_name = (output_mod_name_f)get_symbol (lib, "output_mod_name");
   plugin->description = plugin->output_mod_description ();
   
   output_mod_list = g_slist_append (output_mod_list, plugin);

   LOG ("Output mod plugin '%s', for '%s' loaded", filename, plugin->description);

   return plugin;
}

/* close and free an open output mod plugin */
void output_mod_plugin_close (output_mod_plugin_s *plugin)
{
   output_mod_list = g_slist_remove (output_mod_list, plugin);
   LOG ("Closing '%s' module", plugin->description);
   g_module_close (plugin->lib);
   g_free (plugin);
}

void output_mod_plugin_close_all_helper (gpointer item, gpointer user_data)
{
   output_mod_plugin_s *plugin = (output_mod_plugin_s *)item;
   LOG ("Closing '%s' module", plugin->description);
   g_module_close (plugin->lib);
   g_free (plugin);
}

/* close all output mod plugins */
void output_mod_plugin_close_all (void)
{
   if (output_mod_list == NULL) {
      LOG ("Output mod plugin list already empty");
      return;
   }

   g_slist_foreach (output_mod_list, output_mod_plugin_close_all_helper, NULL);
}

typedef struct mod_configure_t {
   gint arg1;
   gint arg2;
   gpointer user_data;
} mod_configure_s;

void output_mod_plugin_configure_all_helper (gpointer item, gpointer user_data)
{
   output_mod_plugin_s *plugin = (output_mod_plugin_s *)item;
   mod_configure_s *s = (mod_configure_s *)user_data;

   plugin->output_mod_configure (s->arg1, s->arg2, s->user_data);
}

void output_mod_plugin_configure_all (gint arg1, gint arg2, gpointer user_data)
{
   mod_configure_s s;
   
   if (output_mod_list == NULL) {
      LOG ("Output mod plugin list empty");
      return;
   }

   s.arg1 = arg1;
   s.arg2 = arg2;
   s.user_data = user_data;
   g_slist_foreach (output_mod_list, output_mod_plugin_configure_all_helper, &s);
}

typedef struct mod_run_t {
   guchar *chunk;
   gint size;
} mod_run_s;

void output_mod_plugin_run_all_helper (gpointer item, gpointer user_data)
{
   output_mod_plugin_s *plugin = (output_mod_plugin_s *)item;
   mod_run_s *s = (mod_run_s *)user_data;

   plugin->output_mod_run (s->chunk, s->size);
}

void output_mod_plugin_run_all (guchar *chunk, gint size)
{
   mod_run_s s;
   
   if (output_mod_list == NULL) {
      LOG ("Output mod plugin list empty");
      return;
   }

   s.chunk = chunk;
   s.size = size;
   g_slist_foreach (output_mod_list, output_mod_plugin_run_all_helper, &s);
}

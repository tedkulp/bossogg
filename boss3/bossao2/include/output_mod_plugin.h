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

 * This header shows the interface for output  modification plugins
 
*/

#import "common.h"
#import "bossao.h"

typedef gchar *(*output_mod_name_f)(void);
typedef gchar *(*output_mod_description_f)(void);
typedef void (*output_mod_configure_f)(gint arg1, gint arg2, gpointer user_data);
typedef void (*output_mod_get_config_f)(gint *arg1, gint *arg2, gpointer *user_data);
typedef void (*output_mod_run_f)(gchar *chunk, gint size);

typedef struct output_mod_plugin_t {
   output_mod_name_f output_mod_name;
   output_mod_description_f output_mod_description;
   output_mod_configure_f output_mod_configure;
   output_mod_get_config_f output_mod_get_config;
   output_mod_run_f output_mod_run;
   GModule *lib;
   gchar *description;
} output_mod_plugin_s;

void output_mod_plugin_clear (output_mod_plugin_s *plugin);
output_mod_plugin_s *output_mod_plugin_open (gchar *filename);
void output_mod_plugin_close (output_mod_plugin_s *plugin);
void output_mod_plugin_close_all (void);
void output_mod_plugin_run_all (gchar *chunk, gint size);
void output_mod_plugin_configure_all (gint arg1, gint arg2, gpointer user_data);

gchar *output_mod_name (void);
gchar *output_mod_description (void);
void output_mod_configure (gint arg1, gint arg2, gpointer user_data);
void output_mod_get_config (gint *arg1, gint *arg2, gpointer *user_data);
void output_mod_run (gchar *chunk, gint size);

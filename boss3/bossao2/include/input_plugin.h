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

 * This header shows the interface for input plugins (ogg, mp3, etc)
 
 */

#import <dlfcn.h>

#import "common.h"

typedef struct song_t {
   gint finished;
   gchar *filename;
   struct input_plugin_t *input_plugin;
   void *private;
} song_s;

/* each input plugin must implement these functions */
typedef gint (*input_identify_f)(gchar *filename);
typedef gint (*input_seek_f)(song_s *song, gdouble length);
typedef gdouble (*input_time_total_f)(song_s *song);
typedef gdouble (*input_time_current_f)(song_s *song);
typedef gchar *(*input_play_chunk_f)(song_s *song, gint *size, gchar *buffer);
typedef song_s *(*input_open_f)(struct input_plugin_t *plugin, gchar *filename);
typedef gint (*input_close_f)(song_s *song);
typedef gchar *(*input_name_f)(void);

typedef struct input_plugin_t {
   input_identify_f input_identify;
   input_seek_f input_seek;
   input_time_total_f input_time_total;
   input_time_current_f input_time_current;
   input_play_chunk_f input_play_chunk;
   input_close_f input_close;
   input_open_f input_open;
   input_name_f input_name;
   GModule *lib;
   gchar *name;
} input_plugin_s;

song_s *song_new (input_plugin_s *plugin, void *data);
void song_free (song_s *song);

void input_plugin_clear (input_plugin_s *plugin);
void input_plugin_set (input_plugin_s *plugin);
input_plugin_s *input_plugin_open (gchar *filename);
void input_plugin_close (input_plugin_s *plugin);
input_plugin_s *input_plugin_find (gchar *filename);
void input_plugin_close_all (void);

gint input_identify (gchar *filename);
gint input_seek (gdouble len);
gdouble input_time_total (void);
gdouble input_time_current (void);
gchar *input_play_chunk (gint *size, gchar *buf);
song_s *input_open (gchar *filename);
gint input_close (void);
gchar *input_name (void);
gchar *input_filename (void);

gint _input_identify (gchar *filename);
gint _input_seek (song_s *song, gdouble len);
gdouble _input_time_total (song_s *song);
gdouble _input_time_current (song_s *song);
gchar *_input_play_chunk (song_s *song, gint *size, gchar *buf);
song_s *_input_open (input_plugin_s *plugin, gchar *filename);
gint _input_close (song_s *song);
gchar *_input_name (void);

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

 * interface to bossao
 
*/

#import <config.h>
#import "bossao.h"
#import "input_plugin.h"
#import "output_plugin.h"
#import "thbuf.h"

#define THBUF_SIZE 256
static gchar chunks[THBUF_SIZE * BUF_SIZE];

static thbuf_t *thbuf;
static GMutex *pause_mutex, *produce_mutex;
static GThread *producer, *consumer;
static gint producer_pos = 0;
static gint consumer_pos = 0;

static gpointer producer_thread (gpointer p)
{
   gint size;
   gchar *chunk;

   while (1) {
      g_mutex_lock (produce_mutex);
      //LOG ("about to produce %d", producer_pos);
      chunk = input_play_chunk (&size, NULL/*&chunks[producer_pos * BUF_SIZE]*/);
      if (chunk == NULL) {
	 LOG ("got a NULL chunk...");
	 g_usleep (10000);
	 continue;
      }
      thbuf_produce (thbuf, chunk, size, producer_pos);
      //LOG ("produced %p, %d %d %d", chunk, size, producer_pos, consumer_pos);
      producer_pos++;
      if (producer_pos >= THBUF_SIZE) {
	 LOG ("producer thread wrapped %d %d", producer_pos, consumer_pos);
	 producer_pos = 0;
      }
      g_mutex_unlock (produce_mutex);
      g_thread_yield ();
   }

   return NULL;
}

static gpointer consumer_thread (gpointer p)
{
   gint size;
   void *data;
   gchar *chunk;

   while (1) {
      g_mutex_lock (pause_mutex);
      //LOG ("consumed %p %d %d %d", data, size, producer_pos, consumer_pos);
      data = thbuf_consume (thbuf, &size, consumer_pos);
      chunk = (gchar *)data;
      if (chunk == NULL || size == 0) {
	 LOG ("got a NULL chunk %p %d...", chunk, size);
	 g_usleep (10000);
	 continue;
      }
      output_plugin_write_chunk_all (chunk, size);
      g_free (chunk);
      consumer_pos++;
      if (consumer_pos >= THBUF_SIZE) {
	 LOG ("consumer thread wrapped %d %d", producer_pos, consumer_pos);
	 consumer_pos = 0;
      }
      g_mutex_unlock (pause_mutex);
      g_thread_yield ();
   }

   return NULL;
}

/* initialize the required plugins */
static void init_plugins (PyObject *cfgparser)
{
   LOG ("TODO: implement config-file based plugin loading");
   input_plugin_open ("input_mp3.la");
   input_plugin_open ("input_ogg.la");
   input_plugin_open ("input_flac.la");

   output_plugin_open ("output_ao.la");
}

void bossao_thread_init (void)
{
#ifdef G_THREADS_IMPL_NONE
   printf ("Your installation of glib2 does not appear to have thread\n"
	   "support enabled. Get a version of glib that does support\n"
	   "threads, make clean && make, and try this again.\n");
   exit (-1);
#else

   g_thread_init (NULL);
   LOG ("Initialized threading support");
#endif
}

inline void bossao_pause (void)
{
   g_mutex_lock (produce_mutex);
   g_mutex_lock (pause_mutex);
}

inline void bossao_unpause (void)
{
   g_mutex_unlock (produce_mutex);
   g_mutex_unlock (pause_mutex);
}

inline void bossao_stop (void)
{
   g_mutex_lock (produce_mutex);
   //LOG ("stopped produce mutex");
   g_mutex_lock (pause_mutex);
   //LOG ("stopped pause mutex");
   thbuf_clear (thbuf);
   //LOG ("cleared");
   input_close ();
   //LOG ("closed");
   consumer_pos = 0;
   producer_pos = 0;
}

/* allocate the song, get the plugins ready */
void bossao_new (PyObject *cfgparser, gchar *filename)
{
   bossao_thread_init ();
   init_plugins (cfgparser);

   output_plugin_open_all (NULL);

   pause_mutex = g_mutex_new ();
   produce_mutex = g_mutex_new ();
   g_mutex_lock (pause_mutex);
   g_mutex_lock (produce_mutex);
   thbuf = thbuf_new (THBUF_SIZE);

   if (filename != NULL)
      bossao_play (filename);
   
   producer = g_thread_create (producer_thread, NULL, TRUE, NULL);
   if (producer_thread == NULL) {
      LOG ("Problem creating producer thread");
      exit (-1);
   }

   consumer = g_thread_create (consumer_thread, NULL, TRUE, NULL);
   if (consumer_thread == NULL) {
      LOG ("Problem creating consumer thread");
      exit (-1);
   }
   
   return;
}

void bossao_join (void)
{
   g_thread_join (producer);
   g_thread_join (consumer);
}

/* free the song, close the plugins */
void bossao_free (void)
{
   input_plugin_close_all ();
   output_plugin_close_all ();

   g_mutex_free (pause_mutex);
   g_mutex_free (produce_mutex);
   thbuf_free (thbuf);
}

gint bossao_play (gchar *filename)
{
   input_plugin_s *plugin = input_plugin_find (filename);
   input_plugin_set (plugin);
   input_open (filename);
   //g_usleep (1000);
   LOG ("'%s' has %f total time", filename, input_time_total ());
   g_mutex_unlock (produce_mutex);
   //LOG ("produce mutex unlocked");
   g_usleep (10000);
   //LOG ("done sleeping");
   g_mutex_unlock (pause_mutex);
   //LOG ("pause mutex unlocked");
   
   return 0;
}

/* open the output devices */
void bossao_open (PyObject *cfgparser)
{
   output_plugin_open_all (cfgparser);
}

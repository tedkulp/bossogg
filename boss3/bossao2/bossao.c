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
#import "output_mod_plugin.h"
#import "thbuf.h"

typedef struct chunk_t {
   guchar *chunk;
   gint sample_num;
   gint size;
   gchar eof;
} chunk_s;

//static chunk_s chunks[THBUF_SIZE];

static thbuf_sem_t *eof_sem;

static thbuf_t *thbuf;
static GMutex *pause_mutex;//, *produce_mutex;
static GThread *producer, *consumer;
static gint paused = 1;
static gint stopped = 1;
static gint quit;

static output_mod_plugin_s *softmix_plugin;

/* all the modules use this function to load symbols from dynamic libs */
gpointer get_symbol (GModule *lib, gchar *name)
{
   gpointer symbol;
   gboolean ret;
   ret = g_module_symbol (lib, name, &symbol);
   if (symbol == NULL) {
      LOG ("Couldn't find symbol '%s' in library", name);
   }
   return symbol;
}

static void buffer_free_callback (void *p)
{
   chunk_s *chunk = (chunk_s *)p;
   if (chunk) {
      if (chunk->chunk)
	 g_free (chunk->chunk);
      g_free (chunk);
   }
}

static gpointer producer_thread (gpointer p)
{
   gint size;
   guchar *chunk;
   chunk_s *cur_chunk;
   gint64 sample_num = 0;
   gint64 last_sample_num = -1;
   gchar eof = -1;
   void *buf_p = NULL;
   
   g_usleep (10000);

   while (1) {
      if (eof == -1) {
	 LOG ("waiting on eof sem");
	 semaphore_p (eof_sem);
	 eof = 0;
      }
      
      //g_mutex_lock (produce_mutex);
      chunk = input_play_chunk (&size, &sample_num, &eof);
      //g_mutex_unlock (produce_mutex);
      cur_chunk = (chunk_s *)g_malloc (sizeof (chunk_s));
      cur_chunk->chunk = chunk;
      cur_chunk->size = size;
      cur_chunk->sample_num = sample_num;

      if (chunk == NULL) {
	 LOG ("got a NULL chunk: %lld %lld", last_sample_num, sample_num);
	 if (eof) {
	    LOG ("was eof...");
	 } else {
	    g_free (cur_chunk);
	    g_usleep (10000);
	    continue;
	 }
      }
      cur_chunk->eof = eof;
      if (quit) {
	 g_usleep (100000);
	 thbuf_produce (thbuf, cur_chunk);
	 LOG ("stopping thread");
	 g_thread_exit (NULL);
      }
      //LOG ("producing");
      //g_mutex_lock (produce_mutex);
      buf_p = (void *)thbuf_produce (thbuf, cur_chunk);
      //g_mutex_unlock (produce_mutex);
      buffer_free_callback (buf_p);
      //g_usleep (0);
      if (eof) {
	 LOG ("chunk was eof, waiting on semaphore");
	 semaphore_p (eof_sem);
	 eof = 0;
      }

      last_sample_num = sample_num;
      
      if (quit) {
	 g_usleep (10000);
	 LOG ("stopping thread");
	 g_thread_exit (NULL);
      }
   }

   return NULL;
}

static gpointer consumer_thread (gpointer p)
{
   chunk_s *chunk = (chunk_s *)0x1;
   gint64 last_sample_num = -1;
   gint count;

   g_usleep (100000);
   
   while (1) {
      //LOG ("consuming");
      g_mutex_lock (pause_mutex);
      if (thbuf_current_size (thbuf))
	 chunk = (chunk_s *)thbuf_consume (thbuf, &count);
      else {
	 g_mutex_unlock (pause_mutex);
	 g_usleep (10000);
	 continue;
      }
      g_mutex_unlock (pause_mutex);
      if (chunk == NULL) {
	 LOG ("got a NULL struct");
	 g_usleep (100000);
	 continue;
      }
      if (chunk->chunk == NULL) {
	 LOG ("got a NULL chunk %d %d eof %d", (gint)last_sample_num,
	      (gint)input_plugin_samples_total (), chunk->eof);
	 if (last_sample_num == chunk->sample_num) {
	    LOG ("was eof?: %d", chunk->eof);
	    input_plugin_set_end_of_file ();
	    g_usleep (100000);
	 }
	 last_sample_num = chunk->sample_num;
	 g_usleep (10000);
	 if (!chunk->eof) {
	    LOG ("wasn't eof");
	    //g_free (chunk);
	    continue;
	 } 
      }
      if (chunk->eof) {
	 LOG ("got EOF");
	 input_plugin_set_end_of_file ();
	 g_usleep (10000);
	 //g_free (chunk);
	 continue;
      }

      //LOG ("about to play %p of %d size", chunk->chunk, chunk->size);
      output_mod_plugin_run_all (chunk->chunk, chunk->size);
      //g_mutex_lock (pause_mutex);
      output_plugin_write_chunk_all (chunk->chunk, chunk->size);
      //semaphore_p (thbuf->full);
      semaphore_v (thbuf->empty);
      //g_mutex_unlock (pause_mutex);
      last_sample_num = chunk->sample_num;
      //g_free (chunk->chunk);
      //g_free (chunk);
      // give up the scheduler
      //g_usleep (0);
      if (quit) {
	 g_usleep (10000);
	 thbuf_consume (thbuf, &count);
	 LOG ("stopping thread");
	 g_thread_exit (NULL);
      }
      // give up the context in case the producer needs to get ahead
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
   output_plugin_open ("output_shout.la");
   //output_plugin_open ("output_alsa.la");

   softmix_plugin = output_mod_plugin_open ("output_mod_softmix.la");
}

static void bossao_thread_init (void)
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

void bossao_pause (void)
{
   if (paused == 0) {
      g_mutex_lock (pause_mutex);
      paused = 1;
   }
}

void bossao_unpause (void)
{
   if (paused == 1) {
      g_mutex_unlock (pause_mutex);
      paused = 0;
   }
}

void bossao_stop (void)
{
   /*
   LOG ("in stop");
   if (stopped == 0) {
      g_mutex_lock (produce_mutex);
      LOG ("locked produce mutex");
      stopped = 1;
   } else {
      LOG ("already stopped");
      }
   */
   /*
   if (paused == 0) {
      g_mutex_lock (pause_mutex);
      LOG ("locked pause mutex");
      paused = 1;
   } else {
      LOG ("already paused");
      }
   */
   bossao_pause ();
   LOG ("about to clear");
   thbuf_clear (thbuf);
   LOG ("cleared");
   input_close ();
   LOG ("closed");
}

/* allocate the song, get the plugins ready */
void bossao_new (PyObject *cfgparser, gchar *filename)
{
   bossao_thread_init ();

   eof_sem = semaphore_new (0);
   
   init_plugins (cfgparser);

   output_plugin_open_all (cfgparser);

   pause_mutex = g_mutex_new ();
   //produce_mutex = g_mutex_new ();
   thbuf = thbuf_new (THBUF_SIZE);
   thbuf_set_free_callback (thbuf, buffer_free_callback);

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
   quit = 1;
   g_usleep (100000);
   LOG ("stopping");
   bossao_stop ();
   LOG ("stopped");
   input_plugin_close_all ();
   LOG ("input plugins closed");
   output_plugin_close_all ();
   LOG ("output plugins closed");
   bossao_join ();
   LOG ("joined");

   //g_mutex_lock (pause_mutex);
   g_mutex_free (pause_mutex);
   LOG ("pause mutex freed");
   //g_mutex_lock (produce_mutex);
   //g_mutex_free (produce_mutex);
   LOG ("produce mutex freed");
   thbuf_free (thbuf);
   LOG ("thbuf freed");
}

gint bossao_play (gchar *filename)
{
   input_plugin_s *plugin = input_plugin_find (filename);
   input_plugin_set (plugin);
   input_open (plugin, filename);
   stopped = 0;
   semaphore_v (eof_sem);
   //g_mutex_unlock (produce_mutex);
   // give the produce buffer a little time to fill
   g_usleep (100000);
   //paused = 0;
   //g_mutex_unlock (pause_mutex);
   bossao_unpause ();
   
   return 0;
}

/* open the output devices */
void bossao_open (PyObject *cfgparser)
{
   output_plugin_open_all (cfgparser);
}

/* close the output devices */
void bossao_close (void)
{
   output_plugin_close_all ();
}

gint bossao_seek (gdouble secs)
{
   gint ret;
   bossao_pause ();
   //thbuf_clear (thbuf);
   ret = input_seek (secs);
   bossao_unpause ();

   return 0;
}

gdouble bossao_time_total (void)
{
   return input_time_total ();
}

gdouble bossao_time_current (void)
{
   return input_time_current ();
}

gchar *bossao_filename (void)
{
   return input_filename ();
}

gint bossao_finished (void)
{
   return input_finished ();
}

gint bossao_getvol (void)
{
   gpointer vol = NULL;
   if (softmix_plugin != NULL) {
      softmix_plugin->output_mod_get_config (NULL, NULL, &vol);
   }
   return *((gint *)vol);
}

void bossao_setvol (int vol)
{
   if (softmix_plugin != NULL) {
      softmix_plugin->output_mod_configure (0, 0, &vol);
   }
}

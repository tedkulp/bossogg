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

//static chunk_s chunks[THBUF_SIZE];

//static thbuf_sem_t *produce_eof_sem, *consume_eof_sem;
static GStaticMutex produce_eof_sem_mutex = G_STATIC_MUTEX_INIT;
static GStaticMutex consume_eof_sem_mutex = G_STATIC_MUTEX_INIT;
static thbuf_static_sem_t produce_eof_sem, consume_eof_sem;

//static thbuf_t *thbuf;
static GStaticMutex thbuf_mutex = G_STATIC_MUTEX_INIT;
static GStaticMutex thbuf_empty_sem_mutex = G_STATIC_MUTEX_INIT;
static GStaticMutex thbuf_full_sem_mutex = G_STATIC_MUTEX_INIT;
static thbuf_static_sem_t thbuf_empty_sem;
static thbuf_static_sem_t thbuf_full_sem;
static thbuf_static_t thbuf;
//static GMutex *pause_mutex;//, *produce_mutex;
//static thbuf_sem_t *cons_pause_sem, *prod_pause_sem;
static GStaticMutex cons_pause_sem_mutex = G_STATIC_MUTEX_INIT;
static GStaticMutex prod_pause_sem_mutex = G_STATIC_MUTEX_INIT;
static thbuf_static_sem_t cons_pause_sem, prod_pause_sem;
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
   
   //LOG ("in here");
   if (chunk) {
      //g_mutex_lock (chunk->mutex);
      if (chunk->chunk) {
	 g_free (chunk->chunk);
	 chunk->chunk = NULL;
      }
      //g_mutex_unlock (chunk->mutex);
      //g_mutex_free (chunk->mutex);
      g_free (chunk);
      chunk = NULL;
   }
}

static gpointer producer_thread (gpointer p)
{
   gint size;
   gchar *chunk;
   chunk_s *cur_chunk;
   gint64 sample_num = 0;
   gint64 last_sample_num = -1;
   gchar eof = -1;
   gint pos = 0;

   //chunk_s *chunks = (chunk_s *)g_malloc (sizeof (chunk_s) * THBUF_SIZE);
   
   g_usleep (10000);

   if (eof == -1) {
      LOG ("waiting on eof sem");
      //semaphore_p (produce_eof_sem);
      static_semaphore_p (&produce_eof_sem);
      eof = 0;
   }
      
   while (1) {
      //semaphore_p (prod_pause_sem);
      static_semaphore_p (&prod_pause_sem);
      chunk = input_play_chunk (&size, &sample_num, &eof);
      //cur_chunk = &chunks[pos];
      //semaphore_v (prod_pause_sem);
      static_semaphore_v (&prod_pause_sem);
      cur_chunk = (chunk_s *)g_malloc (sizeof (chunk_s));
      cur_chunk->chunk = chunk;
      cur_chunk->size = size;
      cur_chunk->sample_num = sample_num;
      if (quit) {
	 g_usleep (100000);
	 //thbuf_produce (thbuf, cur_chunk);
	 LOG ("stopping thread");
	 g_thread_exit (NULL);
      }
      
      if (chunk == NULL) {
	 LOG ("got a NULL chunk: %lld %lld", last_sample_num, sample_num);
	 g_usleep (10000);
	 if (eof) {
	    LOG ("was eof...");
	    cur_chunk->size = 0;
	    cur_chunk->chunk = NULL;
	 } else {
	    g_free (cur_chunk);
	    //buffer_free_callback (cur_chunk);
	    g_usleep (10000);
	    continue;
	 }
      }
      cur_chunk->eof = eof;
      //LOG ("producing");
      //thbuf_produce (thbuf, cur_chunk);
      pos = thbuf_static_produce (&thbuf, cur_chunk);

      //buffer_free_callback (old_chunk);
      if (eof) {
	 LOG ("chunk was eof, waiting on semaphore");
	 //semaphore_p (produce_eof_sem);
	 static_semaphore_p (&produce_eof_sem);
	 eof = 0;
      }

      last_sample_num = sample_num;
   }

   return NULL;
}

static gpointer consumer_thread (gpointer p)
{
   chunk_s *chunk = (chunk_s *)0x1;
   gint64 last_sample_num = -1;
   gint count;
   gint size;
   
   g_usleep (100000);

   //semaphore_p (consume_eof_sem);
   static_semaphore_p (&consume_eof_sem);

   LOG ("done waiting");
   
   while (1) {
      //semaphore_p (cons_pause_sem);
      static_semaphore_p (&cons_pause_sem);
      //size = thbuf_current_size (thbuf);
      size = thbuf_static_current_size (&thbuf);
      //chunk = (chunk_s *)thbuf_consume (thbuf, &count);
      chunk = (chunk_s *)thbuf_static_consume (&thbuf, &count);
      //semaphore_v (cons_pause_sem);
      static_semaphore_v (&cons_pause_sem);

      if (chunk == NULL) {
	 LOG ("got a NULL struct");
	 //semaphore_v (thbuf->empty);
	 g_usleep (10000);
	 continue;
      }
      if (chunk->chunk == NULL || chunk->size == 0) {
	 LOG ("got a NULL chunk %d %d eof %d count %d size %d", (gint)last_sample_num,
	      (gint)input_plugin_samples_total (), chunk->eof, count, size);
	 if (last_sample_num == chunk->sample_num) {
	    LOG ("was eof?: %d", chunk->eof);
	 }
	 last_sample_num = chunk->sample_num;
	 if (chunk->chunk)
	    g_free (chunk->chunk);
	 g_free (chunk);
	 if (!chunk->eof) {
	    LOG ("wasn't eof");
	    g_usleep (0);
	    continue;
	 } else {
	    LOG ("got eof 1");
	    input_plugin_set_end_of_file ();
	    g_usleep (50000);
	    static_semaphore_p (&consume_eof_sem);
	    continue;
	 }
      }

      if (chunk->eof)
	 input_plugin_set_end_of_file ();
      
      //LOG ("about to play %p of %d size buf size is %d", chunk->chunk, chunk->size, size);
      if (size == 1 || last_sample_num == chunk->sample_num) {
	 LOG ("avoided weirdness: %d", size);
	 if (chunk->eof) {
	    LOG ("got EOF 3");
	    g_free (chunk->chunk);
	    g_free (chunk);
	    input_plugin_set_end_of_file ();
	    g_usleep (50000);
	    //semaphore_p (consume_eof_sem);
	    static_semaphore_p (&consume_eof_sem);
	    continue;
	 }
	 g_free (chunk->chunk);
	 g_free (chunk);
	 continue;
      }
      output_mod_plugin_run_all (chunk->chunk, chunk->size);
      output_plugin_write_chunk_all (chunk->chunk, chunk->size);
      last_sample_num = chunk->sample_num;

      if (chunk->eof) {
	 LOG ("got EOF 2");
	 g_free (chunk->chunk);
	 g_free (chunk);
	 input_plugin_set_end_of_file ();
	 g_usleep (50000);
	 //semaphore_p (consume_eof_sem);
	 static_semaphore_p (&consume_eof_sem);
	 //g_usleep (10000);
	 continue;
      }

      g_free (chunk->chunk);
      g_free (chunk);

      if (quit) {
	 g_usleep (10000);
	 //thbuf_consume (thbuf, &count);
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
   //if (cons_pause_sem->count != 0) {
   if (cons_pause_sem.count != 0) {
      //semaphore_p (cons_pause_sem);
      static_semaphore_p (&cons_pause_sem);
      //cons_pause_sem->count = 0;
      cons_pause_sem.count = 0;
   }
   //if (prod_pause_sem->count != 0) {
   if (prod_pause_sem.count != 0) {
      //semaphore_p (prod_pause_sem);
      static_semaphore_p (&prod_pause_sem);
      //prod_pause_sem->count = 0;
      prod_pause_sem.count = 0;
   }
}

void bossao_unpause (void)
{
   //cons_pause_sem->count = 0;
   cons_pause_sem.count = 0;
   //semaphore_v (cons_pause_sem);
   static_semaphore_v (&cons_pause_sem);
   //prod_pause_sem->count = 0;
   prod_pause_sem.count = 0;
   //semaphore_v (prod_pause_sem);
   static_semaphore_v (&prod_pause_sem);
}

gint first = 1;

void bossao_stop (void)
{
   if (first == 0) {
      bossao_pause ();
   } else
      first = 0;
   LOG ("about to clear");
   //thbuf_clear (thbuf);
   thbuf_static_clear (&thbuf);
   LOG ("cleared");
   input_close ();
   LOG ("closed");
}

/* allocate the song, get the plugins ready */
void bossao_new (PyObject *cfgparser, gchar *filename)
{
   bossao_thread_init ();

   static_semaphore_new (&produce_eof_sem, 0, &produce_eof_sem_mutex);
   static_semaphore_new (&consume_eof_sem, 0, &consume_eof_sem_mutex);
   
   init_plugins (cfgparser);

   output_plugin_open_all (cfgparser);

   //pause_mutex = g_mutex_new ();
   //prod_pause_sem = semaphore_new (0);
   //cons_pause_sem = semaphore_new (0);
   static_semaphore_new (&prod_pause_sem, 0, &prod_pause_sem_mutex);
   static_semaphore_new (&cons_pause_sem, 0, &cons_pause_sem_mutex);
   //produce_mutex = g_mutex_new ();
   //thbuf = thbuf_new (THBUF_SIZE);
   thbuf_static_new (&thbuf, THBUF_SIZE, &thbuf_mutex,
		     &thbuf_empty_sem, &thbuf_empty_sem_mutex,
		     &thbuf_full_sem, &thbuf_full_sem_mutex);
   //thbuf_set_free_callback (thbuf, buffer_free_callback);
   thbuf_static_set_free_callback (&thbuf, buffer_free_callback);

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
   //g_mutex_free (pause_mutex);
   //semaphore_free (prod_pause_sem);
   //semaphore_free (cons_pause_sem);
   static_semaphore_free (&prod_pause_sem);
   static_semaphore_free (&cons_pause_sem);
   LOG ("pause mutex freed");
   //g_mutex_lock (produce_mutex);
   //g_mutex_free (produce_mutex);
   LOG ("produce mutex freed");
   //thbuf_free (thbuf);
   thbuf_static_free (&thbuf);
   LOG ("thbuf freed");
}

gint bossao_play (gchar *filename)
{
   input_plugin_s *plugin = input_plugin_find (filename);
   input_plugin_set (plugin);
   input_open (plugin, filename);
   //bossao_pause ();
   stopped = 0;
   //produce_eof_sem->count = 0;
   //consume_eof_sem->count = 0;
   produce_eof_sem.count = 0;
   consume_eof_sem.count = 0;
   g_usleep (10000);
   //semaphore_v (produce_eof_sem);
   static_semaphore_v (&produce_eof_sem);
   g_usleep (10000);
   //semaphore_v (consume_eof_sem);
   static_semaphore_v (&consume_eof_sem);
   //g_mutex_unlock (produce_mutex);
   // give the produce buffer a little time to fill
   g_usleep (50000);
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

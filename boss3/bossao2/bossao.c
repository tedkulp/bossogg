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

static thbuf_t *thbuf;
static GMutex *pause_mutex, *produce_mutex;
static GThread *producer, *consumer;
static gint producer_pos;
static gint consumer_pos;
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

static gpointer producer_thread (gpointer p)
{
   gint size;
   guchar *chunk;
   chunk_s *cur_chunk;
   gint64 sample_num = 0;
   gint64 last_sample_num = -1;
   gchar eof = 0;
   gchar eof_once = 0;
   gchar *filename;
   gchar *last_filename = NULL;

   g_usleep (10000);
   
   while (1) {
      g_mutex_lock (produce_mutex);
      filename = input_filename ();
      if (last_filename == NULL)
	 last_filename = input_filename ();
      chunk = input_play_chunk (&size, &sample_num, &eof);
      g_mutex_unlock (produce_mutex);
      cur_chunk = (chunk_s *)g_malloc (sizeof (chunk_s));
      cur_chunk->chunk = chunk;
      cur_chunk->size = size;
      cur_chunk->sample_num = sample_num;

      if (chunk == NULL) {
	 LOG ("got a NULL chunk: %lld %lld", last_sample_num, sample_num);
	 if (eof)
	    LOG ("was eof...");
      }

      if (eof_once) {
	 eof_once = 0;
	 if (eof) {
	    g_usleep (100000);
	    continue;
	 }
      }

      if (eof) {
	 LOG ("got eof");
	 if (last_sample_num > 0) {
	    if (eof_once != 1) {
	       eof = 1;
	       eof_once = 1;
	    } else
	       eof = 0;
	 }
	 if (filename != last_filename) {
	    last_filename = filename;
	    //eof_once = 0;
	 }
      } else {
	 if (last_sample_num == sample_num && last_sample_num > 0) {
	    LOG ("sample EOF: %lld %lld", last_sample_num, sample_num);
	    eof = 1;
	 }
      }
      cur_chunk->eof = eof;
      if (eof_once) {
	 LOG ("sleeping...");
	 thbuf_produce (thbuf, cur_chunk, producer_pos);
	 producer_pos++;
	 producer_pos %= THBUF_SIZE;
	 while (last_filename == input_filename ())
	    g_usleep (1000);
	 g_usleep (100000);
	 //eof_once = 0;
	 LOG ("continuing");
	 continue;
      }
      
      if (quit) {
	 g_usleep (100000);
	 thbuf_produce (thbuf, cur_chunk, producer_pos);
	 LOG ("stopping thread");
	 g_thread_exit (NULL);
      }
      if (chunk == NULL) {
	 LOG ("got a NULL chunk...");
	 //g_free (cur_chunk);
	 g_mutex_lock (produce_mutex);
	 thbuf_produce (thbuf, cur_chunk, producer_pos);
	 producer_pos++;
	 producer_pos %= THBUF_SIZE;
	 g_mutex_unlock (produce_mutex);
	 g_usleep (40000);
	 continue;
      }
      //LOG ("producing");
      g_mutex_lock (produce_mutex);
      thbuf_produce (thbuf, cur_chunk, producer_pos);
      /*
      if (eof) {
	 if (!eof_once) {
	    g_usleep (40000);
	    eof = 0;
	    eof_once = 1;
	 }
	 }
      */
      //LOG ("produced %p, %d %d %d", chunk, size, producer_pos, consumer_pos);
      producer_pos++;
      producer_pos %= THBUF_SIZE;
      g_mutex_unlock (produce_mutex);

      last_sample_num = sample_num;
      
      if (producer_pos == 0) {
	 LOG ("producer thread wrapped %d %d", producer_pos, consumer_pos);
      }
      g_usleep (0);
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

   g_usleep (100000);
   
   while (1) {
      //LOG ("consuming");
      g_mutex_lock (pause_mutex);
      chunk = (chunk_s *)thbuf_consume (thbuf, consumer_pos);
      consumer_pos++;
      consumer_pos %= THBUF_SIZE;
      g_mutex_unlock (pause_mutex);
      if (chunk == NULL) {
	 LOG ("got a NULL struct");
	 g_usleep (100000);
	 continue;
      }
      if (chunk->chunk == NULL) {
	 LOG ("got a NULL chunk %d %d", (gint)last_sample_num, (gint)input_plugin_samples_total ());
	 if (last_sample_num == chunk->sample_num) {
	    LOG ("was eof?");
	    input_plugin_set_end_of_file ();
	    g_usleep (100000);
	 }
	 last_sample_num = chunk->sample_num;
	 g_usleep (10000);
	 if (!chunk->eof) {
	    LOG ("wasn't eof");
	    continue;
	 }
      }
      if (chunk->eof) {
	 LOG ("got EOF");
	 input_plugin_set_end_of_file ();
	 g_usleep (100000);
	 continue;
      }

      if (consumer_pos == 0) {
	 LOG ("consumer thread wrapped %d %d", producer_pos, consumer_pos);
      }       
      //LOG ("about to play %p of %d size", chunk->chunk, chunk->size);
      g_mutex_lock (pause_mutex);
      output_mod_plugin_run_all (chunk->chunk, chunk->size);
      g_mutex_unlock (pause_mutex);
      output_plugin_write_chunk_all (chunk->chunk, chunk->size);
      last_sample_num = chunk->sample_num;
      g_free (chunk->chunk);
      g_free (chunk);
      g_usleep (0);
      if (quit) {
	 g_usleep (10000);
	 //thbuf_consume (thbuf, consumer_pos);
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
   if (stopped == 0) {
      g_mutex_lock (produce_mutex);
      stopped = 1;
   }
   if (paused == 0) {
      g_mutex_lock (pause_mutex);
      paused = 1;
   }
   thbuf_clear (thbuf);
   input_close ();
   consumer_pos = 0;
   producer_pos = 0;
}

/* allocate the song, get the plugins ready */
void bossao_new (PyObject *cfgparser, gchar *filename)
{
   bossao_thread_init ();
   init_plugins (cfgparser);

   output_plugin_open_all (cfgparser);

   pause_mutex = g_mutex_new ();
   produce_mutex = g_mutex_new ();
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
   quit = 1;
   g_usleep (100000);
   LOG ("stopping");
   bossao_stop ();
   LOG ("stopped");
   //bossao_join ();
   //LOG ("joined");
   
   input_plugin_close_all ();
   output_plugin_close_all ();
   LOG ("plugins closed");

   //g_mutex_lock (pause_mutex);
   g_mutex_free (pause_mutex);
   LOG ("pause mutex freed");
   //g_mutex_lock (produce_mutex);
   g_mutex_free (produce_mutex);
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
   g_mutex_unlock (produce_mutex);
   // give the produce buffer a little time to fill
   g_usleep (100000);
   paused = 0;
   g_mutex_unlock (pause_mutex);

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

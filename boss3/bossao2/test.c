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

 * test program for bossao2 (woohoo!)
 
*/

#import "bossao.h"
#import "input_plugin.h"
#import "output_plugin.h"
#import "thbuf.h"

thbuf_t *thbuf;
GMutex *cons_pause_mutex, *prod_pause_mutex;
int prod_pos, cons_pos;

#define THBUF_SIZE 64

gchar chunks[THBUF_SIZE * BUF_SIZE];

static gpointer producer (gpointer p)
{
   //int pos = 0;
   LOG ("producer thread started");

   while (1) {
      //g_mutex_lock (prod_pause_mutex);
      int size;
      gchar *chunk = input_play_chunk (NULL, &size, &chunks[prod_pos * BUF_SIZE]);
      //LOG ("producing");
      //output_plugin_write_chunk_all (NULL, chunk, size);
      //thbuf_produce (thbuf, chunk, size, pos);
      //LOG ("in chunk is %d %p", size, chunk);
      thbuf_produce (thbuf, chunk, size, prod_pos);
      //if (chunk == NULL) {
      // LOG ("got a null chunk, ending thread");
      // g_thread_exit (0);
      //}
      //LOG ("done producing");
      /* uncomment next line to have the producer play audio (single-threaded) */
      //pos++;
      prod_pos++;
      //if (pos >= THBUF_SIZE) {
      if (prod_pos >= THBUF_SIZE) {
	 LOG ("producer thread wrapped %d %d", prod_pos, cons_pos);
	 //pos = 0;
	 prod_pos = 0;
      }
      //if (cons_pos == prod_pos + 1)
//	 g_usleep (10000);
      //g_mutex_unlock (prod_pause_mutex);
      g_thread_yield ();
   }
      
   LOG ("producer thread done");

   return NULL;
}

static gpointer consumer (gpointer p)
{
   int size;
   void *data;
   //int pos = 0;
   
   LOG ("consumer thread started");

   while (1) {
      g_mutex_lock (cons_pause_mutex);
      gchar *chunk;
      //LOG ("consuming");
      //data = thbuf_consume (thbuf, &size, pos);
      data = thbuf_consume (thbuf, &size, cons_pos);
      g_mutex_unlock (cons_pause_mutex);
      //LOG ("done consuming");
      chunk = (gchar *)data;
      //if (chunk == NULL) {
      // LOG ("got a null chunk, ending thread");
      // g_thread_exit (0);
      //}
      /* comment this next line if you want the producer to play audio */
      //LOG ("out chunk is %d %p", size, chunk);
      if (chunk != NULL && size != 0)
	 output_plugin_write_chunk_all (NULL, chunk, size);
      g_free (chunk);
      //pos++;
      cons_pos++;
      //if (pos >= THBUF_SIZE) {
      if (cons_pos >= THBUF_SIZE) {
	 LOG ("consumer thread wrapped %d %d", prod_pos, cons_pos);
	 //pos = 0;
	 cons_pos = 0;
      }
      g_thread_yield ();
   }

   LOG ("consumer thread done, got to %d", cons_pos);

   return NULL;
}

int main (int argc, char *argv[])
{
   
   input_plugin_s *plugin;
   song_s *song;

   if (argc != 4) {
      printf ("Usage: %s <filename1.ogg> <filename2.ogg> <secs-to-sleep>\n", argv[0]);
      exit (-1);
   }

   int secs_to_sleep = atoi (argv[3]);
   printf ("Sleeping for %d seconds between operations\n", secs_to_sleep);

   bossao_thread_init ();

   cons_pause_mutex = g_mutex_new ();
   prod_pause_mutex = g_mutex_new ();
   
   input_plugin_open ("input_mp3.la");
   input_plugin_open ("input_ogg.la");
   input_plugin_open ("input_flac.la");

   output_plugin_open ("output_ao.la");
   //output_plugin_open ("output_file.la");

   plugin = input_plugin_find (argv[1]);
   input_plugin_set (plugin);

   output_plugin_open_all (NULL);
   input_open (song, argv[1]);
   LOG ("%s has %f total time", argv[1], input_time_total (song));
   gint size;
   
   thbuf = thbuf_new (THBUF_SIZE);
   GThread *producer_thread, *consumer_thread;
   producer_thread = g_thread_create (producer, NULL, TRUE, NULL);
   if (producer_thread == NULL) {
      LOG ("Problem creating producer thread");
      exit (-6);
   }
   consumer_thread = g_thread_create (consumer, NULL, TRUE, NULL);
   if (consumer_thread == NULL) {
      LOG ("Problem creating consumer thread");
      exit (-6);
   }

   /* test pausing */
   sleep (secs_to_sleep);
   LOG ("pausing");
   g_mutex_lock (cons_pause_mutex);
   sleep (secs_to_sleep);
   LOG ("unpausing");
   g_mutex_unlock (cons_pause_mutex);

   /* test stopping and loading a new file */
   
   sleep (secs_to_sleep);
   LOG ("clearing");
   //g_mutex_lock (prod_pause_mutex);
   g_mutex_lock (cons_pause_mutex);
   LOG ("about the thbuf_clear");
   thbuf_clear (thbuf);
   prod_pos = 0;
   cons_pos = 0;
   //g_thread_yield ();
   LOG ("done clearing");
   //sleep (secs_to_sleep);
   //g_usleep (10);
   LOG ("done sleeping");
   LOG ("closing input");
   input_close (song);
   LOG ("done closing");
   plugin = input_plugin_find (argv[2]);
   input_plugin_set (plugin);
   input_open (song, argv[2]);
   g_usleep (100000);
   //g_mutex_unlock (prod_pause_mutex);
   g_mutex_unlock (cons_pause_mutex);
   //sleep (secs_to_sleep);
   

   LOG ("joining..");
   
   g_thread_join (producer_thread);
   g_thread_join (consumer_thread);

   LOG ("threads have joined");
   
   input_close (song);
   input_plugin_close_all ();
   output_plugin_close_all ();
   
   thbuf_free (thbuf);
   g_mutex_free (cons_pause_mutex);
   g_mutex_free (prod_pause_mutex);
   
   return 0;
}

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

thbuf_t *thbuf;

#define THBUF_SIZE 256

static gpointer producer (gpointer p)
{
   int pos = 0;
   LOG ("producer thread started");

   while (1) {
      int size;
      gchar *chunk = input_play_chunk (NULL, &size);
      LOG ("producing");
      thbuf_produce (thbuf, chunk, size, pos);
      //LOG ("done producing");
      /* uncomment next line to have the producer play audio (single-threaded) */
      //output_plugin_write_chunk_all (NULL, chunk, size);
      pos++;
      if (pos >= THBUF_SIZE) {
	 LOG ("producer thread wrapped");
	 pos = 0;
      }
   }
      
   LOG ("producer thread done");

   return NULL;
}

static gpointer consumer (gpointer p)
{
   int size;
   void *data;
   int pos = 0;
   
   LOG ("consumer thread started");

   while (1) {
      gchar *chunk;
      //LOG ("consuming");
      data = thbuf_consume (thbuf, &size, pos);
      //LOG ("done consuming");
      chunk = (gchar *)data;
      /* comment this next line if you want the producer to play audio */
      output_plugin_write_chunk_all (NULL, chunk, size);
      g_free (chunk);
      pos++;
      if (pos >= THBUF_SIZE) {
	 LOG ("consumer thread wrapped");
	 pos = 0;
      }
   }

   LOG ("consumer thread done, got to %d", pos);

   return NULL;
}

int main (int argc, char *argv[])
{
   
   input_plugin_s *plugin;
   song_s *song;

   if (argc != 2) {
      printf ("Usage: %s <filename.ogg>\n", argv[0]);
      exit (-1);
   }

   bossao_thread_init ();
   
   input_plugin_open ("input_mp3.la");
   input_plugin_open ("input_ogg.la");
   input_plugin_open ("input_flac.la");

   output_plugin_open ("output_ao.la");
   //output_plugin_open ("output_file.la");

   plugin = input_plugin_find (argv[1]);
   input_plugin_set (plugin);

   output_plugin_open_all (NULL);
   input_open (song, argv[1]);
   LOG ("test.ogg has %f total time", input_time_total (song));
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

   while (1)
      sleep (1);
   
   sleep (2);
   thbuf_clear (thbuf);
   LOG ("cleared");
   sleep (2);
   input_close (song);
   input_open (song, "/home/adam/test.ogg");

   LOG ("about to do it...");
   
   g_mutex_lock (thbuf->empty->mutex);
   g_cond_broadcast (thbuf->empty->cond);
   g_mutex_unlock (thbuf->empty->mutex);
   g_mutex_lock (thbuf->full->mutex);
   g_cond_broadcast (thbuf->full->cond);
   g_mutex_unlock (thbuf->full->mutex);

   LOG ("joining..");
   
   g_thread_join (producer_thread);
   g_thread_join (consumer_thread);

   LOG ("threads have joined");
   
   input_close (song);
   input_plugin_close_all ();
   output_plugin_close_all ();
   
   thbuf_free (thbuf);
   
   return 0;
   
   /* OLD STUFF
  song_s *testsong = NULL;
  int i = 0;

  printf("Creating new struct\n");
  testsong = bossao_new();
  printf ("preparing to start\n");
  bossao_start (testsong, NULL);
  char *filename = (char *)malloc (sizeof ("/home/adam/bbking.flac"));
  strcpy (filename, "/home/adam/test.mp3");
  char *filename2 = strdup (filename);
  printf ("filename is %s\n", filename);
  printf ("filename has %p\n", filename);
  printf ("playing first file\n");
  bossao_play (testsong, filename);
  printf ("Driver name is %s\n", bossao_driver_name (testsong));
  
  while (i < 3) {
    usleep (1000000);
    i++;
  }

  printf ("pausing\n");
  bossao_pause (testsong);
  sleep (2);
  printf ("unpausing\n");
  bossao_unpause (testsong);

  while (i < 4) {
    usleep (1000000);
    i++;
  }

  bossao_stop (testsong);
  printf ("stopped\n");
  sleep (2);
  printf ("readying the play: %s\n", filename2);
  bossao_play (testsong, filename2);
  sleep (4);
	
  bossao_shutdown (testsong);
	
  usleep (200000);
	
  printf ("closing\n");

  printf("Deleting struct\n");
  bossao_del(testsong);
  free (filename);
	
  return 0;
   */
}

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

 * shoutcast output plugin
 
*/

#import "output_plugin.h"

#import <shout/shout.h>
#import <vorbis/vorbisenc.h>
#import <vorbis/codec.h>
#import <vorbis/vorbisfile.h>

static gchar *plugin_name = "shout";
static gchar *driver_name = "Shoutcast Ogg Encoder";

static shout_t *st;

static ogg_stream_state os;
static ogg_page og;
static ogg_packet op;
static vorbis_dsp_state vd;
static vorbis_block vb;
static vorbis_info vi;
static vorbis_comment vc;

static ogg_packet header_main;
static ogg_packet header_comments;
static ogg_packet header_codebooks;

static gint channels;
static gint rate;
//static gint sample_size;
//static gint bytes_per_sec;

static gint write_page (ogg_page *page)
{
   gint written;

   shout_sync (st);
   written = shout_send (st, page->header, page->header_len);
   written = shout_send (st, page->body, page->body_len);
   shout_sync (st);

   return written;
}

static void ready_encoder (void)
{
   channels = 2;
   rate = 44100;
   gint bitrate;
   gint serialno = rand ();
   gint result, ret;
   gchar *title = NULL;

   bitrate = 196;
   title = strdup ("Bossogg Streaming Media Server");
  
   vorbis_info_init (&vi);
  
   if (vorbis_encode_setup_managed (&vi, channels, rate, -1, bitrate * 1000, -1)) {
      printf ("Failed to setup encoder\n");
      vorbis_info_clear (&vi);
      return;
   }

   /* not needed, according to af_
    */
   vorbis_encode_ctl (&vi, OV_ECTL_RATEMANAGE_SET, NULL);
  
   vorbis_encode_setup_init (&vi);
   vorbis_analysis_init (&vd, &vi);
   vorbis_block_init (&vd, &vb);

   ogg_stream_init (&os, serialno);

   vorbis_comment_init (&vc);
   vorbis_comment_add_tag (&vc, "title", title);
   vorbis_analysis_headerout (&vd, &vc, &header_main,
			      &header_comments, &header_codebooks);

   ogg_stream_packetin(&os,&header_main);
   ogg_stream_packetin(&os,&header_comments);
   ogg_stream_packetin(&os,&header_codebooks);

   while ((result = ogg_stream_flush (&os, &og))) {
      if (result == 0)
	 break;
      ret = write_page (&og);
   }

   if (title != NULL)
      g_free (title);

   LOG ("encoder ready");
   
   return;
}

gint output_open (PyObject *cfgparser)
{
   //shout_t *shout;
   PyObject *temp;

   gchar *host, *user, *password, *mount;
   int port;
   
   if (cfgparser == NULL) {
      LOG ("Using shout defaults");
      host = strdup ("127.0.0.1");
      user = strdup ("admin");
      password = strdup ("hackme");
      mount = strdup ("/bossogg.ogg");
      port = 8000;
   } else {
      temp = PyObject_CallMethod (cfgparser, "get", "s,s", "host", "SHOUT");
      host = strdup (PyString_AsString (temp));
      Py_DECREF (temp);
    
      temp = PyObject_CallMethod (cfgparser, "get", "s,s", "user", "SHOUT");
      user = strdup (PyString_AsString (temp));
      Py_DECREF (temp);
    
      temp = PyObject_CallMethod (cfgparser, "get", "s,s", "password", "SHOUT");
      password = strdup (PyString_AsString (temp));
      Py_DECREF (temp);
    
      temp = PyObject_CallMethod (cfgparser, "get", "s,s", "mount", "SHOUT");
      mount = strdup (PyString_AsString (temp));
      Py_DECREF (temp);
    
      temp = PyObject_CallMethod (cfgparser, "get", "s,s", "port", "SHOUT");
      port = atoi (PyString_AsString (temp));
      Py_DECREF (temp);
   }

   shout_init ();
   if (!(st = shout_new ())) {
      printf ("Problem allocing shout\n");
      return -1;
   }

   if (shout_set_host (st, host) != SHOUTERR_SUCCESS) {
      printf ("Error setting shout host\n");
      return -1;
   }

   if (shout_set_protocol (st, SHOUT_PROTOCOL_HTTP) != SHOUTERR_SUCCESS) {
      printf ("Error setting http protocol\n");
      return -1;
   }

   if (shout_set_port (st, port) != SHOUTERR_SUCCESS) {
      printf ("Error setting port\n");
      return -1;
   }

   if (shout_set_password (st, password) != SHOUTERR_SUCCESS) {
      printf ("Error setting password\n");
      return -1;
   }

   if (shout_set_mount (st, mount) != SHOUTERR_SUCCESS) {
      printf ("Error setting mount\n");
      return -1;
   }

   if (shout_set_user (st, user) != SHOUTERR_SUCCESS) {
      printf ("Error setting user\n");
      return -1;
   }

   if (shout_set_format (st, SHOUT_FORMAT_VORBIS) != SHOUTERR_SUCCESS) {
      printf ("Error setting vorbis format\n");
      return -1;
   }

   if (shout_open (st) == SHOUTERR_SUCCESS) {
      // don't need to do anything
   } else {
      printf ("Error connecting to shout server\n");
      printf ("error: %s\n", shout_get_error (st));
      return -1;
   }

   g_free (host);
   g_free (user);
   g_free (password);
   g_free (mount);
   
   ready_encoder ();

   return 0;
}

void output_close (void)
{
   ogg_stream_clear (&os);
   vorbis_block_clear (&vb);
   vorbis_dsp_clear (&vd);
   vorbis_comment_clear (&vc);
   vorbis_info_clear (&vi);

   shout_close (st);
   shout_free (st);
}

gint output_write_chunk (gchar *buffer, gint size)
{
   gint i, result, ret, eos = 0;
   gchar *buf = buffer;
   static gint samples_in_page = 0;
   static gint prev_granulepos = 0;
   rate = 44100;

   float **vorbbuf = vorbis_analysis_buffer (&vd, size / 4);
   samples_in_page += size / 4;

   for (i = 0; i < size / 4; i++) {
      vorbbuf[0][i] = ((buf[i * 4 + 1] << 8) | (0x00ff&(gint)buf[i * 4])) / 32768.f;
      vorbbuf[1][i] = ((buf[i * 4 + 3] << 8) | (0x00ff&(gint)buf[i * 4 + 2])) / 32768.f;
   }

   vorbis_analysis_wrote (&vd, size / 4);

   while ((ret = vorbis_analysis_blockout (&vd, &vb)) == 1) {
      vorbis_analysis (&vb, NULL);
      vorbis_bitrate_addblock (&vb);

      while (vorbis_bitrate_flushpacket (&vd, &op)) {
	 ogg_stream_packetin (&os, &op);
	 while (!eos) {
	    if (samples_in_page > rate * 2) {
	       result = ogg_stream_flush (&os, &og);
	       samples_in_page = 0;
	    } else
	       result = ogg_stream_pageout (&os, &og);
	    if (result == 0) {
	       break;
	    }
	    samples_in_page -= ogg_page_granulepos (&og) - prev_granulepos;
	    prev_granulepos = ogg_page_granulepos (&og);
	    ret = write_page (&og);
	    if (ogg_page_eos (&og)) {
	       eos = 1;
	    }
	 }
      }
   }

   return 0;
}

gchar *output_name (void)
{
   return plugin_name;
}

gchar *output_driver_name (void)
{
   return driver_name;
}



#include <config.h>

#include <Python.h>
#include <ao/ao.h>

#ifdef HAVE_SHOUT
#include <shout/shout.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/codec.h>
#endif

#include "bossao.h"

#ifdef HAVE_VORBIS
#include <vorbis/vorbisfile.h>
#include "ogg.h"
#endif

#ifdef HAVE_SHOUT

typedef struct encoder_t {
  ogg_stream_state os;
  ogg_page og;
  ogg_packet op;
  vorbis_dsp_state vd;
  vorbis_block vb;
  vorbis_info vi;
  vorbis_comment vc;

  ogg_packet header_main;
  ogg_packet header_comments;
  ogg_packet header_codebooks;

  int channels;
  int rate;
  int samplesize;
  int bytespersec;
} encoder_s;

static int write_page (ogg_page *page, song_s *song)
{
  int written;

  shout_sync (song->st);
  written = shout_send (song->st, page->header, page->header_len);
  written = shout_send (song->st, page->body, page->body_len);
  shout_sync (song->st);  

  return written;
}

void shout_encode_chunk (song_s *song, unsigned char *buffer, int size)
{
  encoder_s *encoder = song->encoder;
  int i, result, ret, eos = 0;
  signed char *buf = (signed char *)malloc (size * sizeof (signed char));
  static int samples_in_page = 0;
  static int prev_granulepos = 0;
  int rate = RATE;

  for (i = 0; i < size; i++) {
    buf[i] = (signed char)buffer[i];
  }

  float **vorbbuf = vorbis_analysis_buffer (&encoder->vd, size / 4);
  samples_in_page += size / 4;
  
  for (i = 0; i < size / 4; i++) {
    vorbbuf[0][i] = ((buf[i * 4 + 1] << 8) | (0x00ff&(int)buf[i * 4])) / 32768.f;
    vorbbuf[1][i] = ((buf[i * 4 + 3] << 8) | (0x00ff&(int)buf[i * 4 + 2])) / 32768.f;
  }
  
  vorbis_analysis_wrote (&encoder->vd, size / 4);

  while ((ret = vorbis_analysis_blockout (&encoder->vd, &encoder->vb)) == 1) {
    vorbis_analysis (&encoder->vb, NULL);
    vorbis_bitrate_addblock (&encoder->vb);

    while (vorbis_bitrate_flushpacket (&encoder->vd, &encoder->op)) {
      ogg_stream_packetin (&encoder->os, &encoder->op);

      while (!eos) {
	if (samples_in_page > rate * 2) {
	  result = ogg_stream_flush (&encoder->os, &encoder->og);
	  samples_in_page = 0;
	} else
	  result = ogg_stream_pageout (&encoder->os, &encoder->og);
	if (result == 0) {
	  break;
	}
	samples_in_page -= ogg_page_granulepos (&encoder->og) - prev_granulepos;
	prev_granulepos = ogg_page_granulepos (&encoder->og);
	ret = write_page (&encoder->og, song);
	if (ogg_page_eos (&encoder->og)) {
	  eos = 1;
	}
      }
    }
  }
  free (buf);
}

int shout_encoder_init (song_s *song, PyObject *cfgparser)
{
  song->encoder = (encoder_s *)malloc (sizeof (encoder_s));;
  encoder_s *encoder = song->encoder;
  int channels = 2;
  int rate = RATE;
  int bitrate;
  int serialno = rand ();
  int result, ret;
  char *title = NULL;
  PyObject *temp;

  if (cfgparser == NULL) {
    bitrate = 196;
    title = strdup ("Bossogg Streaming Media Server");
  } else {
    temp = PyObject_CallMethod (cfgparser, "get", "s,s", "bitrate", "SHOUT");
    bitrate = atoi (PyString_AsString (temp));
    Py_DECREF (temp);

    temp = PyObject_CallMethod (cfgparser, "get", "s,s", "title", "SHOUT");
    title = PyString_AsString (temp);
    Py_DECREF (temp);
  }

  vorbis_info_init (&encoder->vi);
  
  if (vorbis_encode_setup_managed (&encoder->vi, channels, rate, -1, bitrate * 1000, -1)) {
    printf ("Failed to setup encoder\n");
    vorbis_info_clear (&encoder->vi);
    return -1;
  }

  /* not needed, according to af_
  */
  vorbis_encode_ctl (&encoder->vi, OV_ECTL_RATEMANAGE_SET, NULL);
  
  vorbis_encode_setup_init (&encoder->vi);
  vorbis_analysis_init (&encoder->vd, &encoder->vi);
  vorbis_block_init (&encoder->vd, &encoder->vb);

  ogg_stream_init (&encoder->os, serialno);

  vorbis_comment_init (&encoder->vc);
  vorbis_comment_add_tag (&encoder->vc, "title", title);
  vorbis_analysis_headerout (&encoder->vd, &encoder->vc, &encoder->header_main,
			     &encoder->header_comments, &encoder->header_codebooks);

  ogg_stream_packetin(&encoder->os,&encoder->header_main);
  ogg_stream_packetin(&encoder->os,&encoder->header_comments);
  ogg_stream_packetin(&encoder->os,&encoder->header_codebooks);

  while ((result = ogg_stream_flush (&encoder->os, &encoder->og))) {
    if (result == 0)
      break;
    ret = write_page (&encoder->og, song);
  }

  if (title == NULL)
    free (title);

  return ret;
}

shout_t *bossao_shout_init (PyObject *cfgparser)
{
  shout_t *shout;
  PyObject *temp;

  char *host, *user, *password, *mount;
  int port;

  if (cfgparser == NULL) {
    host = strdup ("127.0.0.1");
    user = strdup ("source");
    password = strdup ("test");
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
  if (!(shout = shout_new ())) {
    printf ("Problem allocing shout\n");
    return NULL;
  }

  if (shout_set_host (shout, host) != SHOUTERR_SUCCESS) {
    printf ("Error setting shout host\n");
    return NULL;
  }

  if (shout_set_protocol (shout, SHOUT_PROTOCOL_HTTP) != SHOUTERR_SUCCESS) {
    printf ("Error setting http protocol\n");
    return NULL;
  }

  if (shout_set_port (shout, port) != SHOUTERR_SUCCESS) {
    printf ("Error setting port\n");
    return NULL;
  }

  if (shout_set_password (shout, password) != SHOUTERR_SUCCESS) {
    printf ("Error setting password\n");
    return NULL;
  }

  if (shout_set_mount (shout, mount) != SHOUTERR_SUCCESS) {
    printf ("Error setting mount\n");
    return NULL;
  }

  if (shout_set_user (shout, user) != SHOUTERR_SUCCESS) {
    printf ("Error setting user\n");
    return NULL;
  }

  if (shout_set_format (shout, SHOUT_FORMAT_VORBIS) != SHOUTERR_SUCCESS) {
    printf ("Error setting vorbis format\n");
    return NULL;
  }

  if (shout_open (shout) == SHOUTERR_SUCCESS) {
    // don't need to do anything
  } else {
    printf ("Error connecting to shout server\n");
    printf ("error: %s\n", shout_get_error (shout));
    return NULL;
  }

  free (host);
  free (user);
  free (password);
  free (mount);

  return shout;
}

void shout_clear (song_s *song)
{
  encoder_s *encoder = song->encoder;
  
  ogg_stream_clear (&encoder->os);
  vorbis_block_clear (&encoder->vb);
  vorbis_dsp_clear (&encoder->vd);
  vorbis_comment_clear (&encoder->vc);
  vorbis_info_clear (&encoder->vi);

  shout_close (song->st);
  shout_free (song->st);
  //shout_shutdown ();
  free (encoder);
}

#endif



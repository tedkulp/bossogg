#include "config.h"

#define PYTHON 1

#include <stdio.h>
#include <pthread.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/codec.h>
#include <cdda_interface.h>
#include <Python.h>

#include "../metadata/id3.h"
#include "rip.h"

static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static stats_s stats;

/* possibly a config option here */
#define READ_SECTORS 64

extern int errno;

static int write_page (ogg_page *page, FILE *fp)
{
  int written;
  written = fwrite (page->header, 1, page->header_len, fp);
  written += fwrite (page->body, 1, page->body_len, fp);

  return written;
}

static int comment_ogg ()
{
  
}

long audio_read_func (cdrom_drive *drive, float **buffer, int prev_sector, int sectors)
{
  int curr_sector;
  
  curr_sector = cdda_read (drive, buffer, prev_sector, sectors);
  prev_sector += curr_sector;


  return curr_sector;
}


static void update_statistics (long total, long done, double time, int tracknum,
			       int tracktot, int finished, char **filenames_c)
{
  pthread_mutex_lock (&stats_mutex);

  stats.done = finished;
  stats.filenames_c = filenames_c;
  int i;
  
  double remain_time;
  int minutes = 0, seconds = 0;
  char buf[256];

  if (finished == 1) {
    stats.minutes = 0;
    stats.seconds = 0;
    stats.tracknum = 0;
    stats.tracktot = tracktot;

    pthread_mutex_unlock (&stats_mutex);
    return;
  }
  
  remain_time = time / ((double)done / (double)total) - time;
  minutes = ((int)remain_time) / 60;
  seconds = (int)(remain_time - (double)((int)remain_time / 60) * 60);

  /* do something interesting with client here */

  stats.minutes = minutes;
  stats.seconds = seconds;
  stats.tracknum = tracknum;
  stats.tracktot = tracktot;

  pthread_mutex_unlock (&stats_mutex);
}

static void update_statistics_track ()
{

  return;
}

static void end_func (double time, int rate, long samples, long bytes)
{
  /* do something here if you want */

  return;
}

static void start_func (char *outfn, int bitrate, float quality, int qset,
			int managed, int min_br, int max_br)
{
  /* do something here if you want */
  
  return;
}

static time_t *timer_start (void)
{
  time_t *start = (time_t *)malloc (sizeof (time_t));
  time (start);
  return start;
}

static double timer_time (time_t *timer)
{
  time_t now = time (NULL);
  time_t start = *((time_t *)timer);

  if (now - start)
    return (double)(now - start);
  else
    return 1;
}

static void timer_clear (time_t *timer)
{
  free (timer);
}

static int encode_ogg (cdrom_drive *drive, rip_opts_s *rip_opts,
		       text_tag_s *text_tag, int track,
		       int tracktot, char *filename, char **filenames)
{
  ogg_stream_state os;
  ogg_page og;
  ogg_packet op;

  vorbis_dsp_state vd;
  vorbis_block vb;
  vorbis_info vi;

  long samplesdone = 0;
  int sector = 0, last_sector = 0;
  long bytes_written = 0, packetsdone = 0;
  double time_elapsed = 0.0;
  int ret = 0;
  time_t *timer;
  double time;
  
  int serialno = rand ();
  vorbis_comment vc;
  long total_samples_per_channel = 0;
  int channels = 2;
  int eos = 0;
  long rate = 44100;
  FILE *out = fopen (filename, "w+");

  timer = timer_start ();

  if (!rip_opts->managed && (rip_opts->min_bitrate > 0 || rip_opts->max_bitrate > 0)) {
    log_msg ("Min or max bitrate requires managed", FL, FN, LN);
    return -1;
  }

  if (rip_opts->bitrate < 0 && rip_opts->min_bitrate < 0 && rip_opts->max_bitrate < 0) {
    rip_opts->quality_set = 1;
  }
  
  start_func (filename, rip_opts->bitrate, rip_opts->quality, rip_opts->quality_set,
	      rip_opts->managed, rip_opts->min_bitrate, rip_opts->max_bitrate);
  
  vorbis_info_init (&vi);

  if (rip_opts->quality_set > 0) {
    if (vorbis_encode_setup_vbr (&vi, channels, rate, rip_opts->quality)) {
      log_msg ("Couldn't initialize vorbis_info", FL, FN, LN);
      vorbis_info_clear (&vi);
      return -1;
    }
    /* two options here, max or min bitrate */
    if (rip_opts->max_bitrate > 0 || rip_opts->min_bitrate > 0) {
      struct ovectl_ratemanage_arg ai;
      vorbis_encode_ctl (&vi, OV_ECTL_RATEMANAGE_GET, &ai);
      ai.bitrate_hard_min = rip_opts->min_bitrate;
      ai.bitrate_hard_max = rip_opts->max_bitrate;
      ai.management_active = 1;
      vorbis_encode_ctl (&vi, OV_ECTL_RATEMANAGE_SET, &ai);
    }
  } else {
    if (vorbis_encode_setup_managed (&vi, channels, rate,
				     rip_opts->max_bitrate > 0 ? rip_opts->max_bitrate * 1000 : -1,
				     rip_opts->bitrate * 1000,
				     rip_opts->min_bitrate > 0 ? rip_opts->min_bitrate * 1000 : -1)) {
      log_msg ("Mode init failed, encode setup managed", FL, FN, LN);
      vorbis_info_clear (&vi);
      return -1;
    }
  }

  if (rip_opts->managed && rip_opts->bitrate < 0) {
    vorbis_encode_ctl (&vi, OV_ECTL_RATEMANAGE_AVG, NULL);
  } else if (!rip_opts->managed) {
    vorbis_encode_ctl (&vi, OV_ECTL_RATEMANAGE_SET, NULL);
  }

  /* set advanced encoder options */

  vorbis_encode_setup_init (&vi);

  vorbis_analysis_init (&vd, &vi);
  vorbis_block_init (&vd, &vb);

  ogg_stream_init (&os, serialno);

  {
    ogg_packet header_main;
    ogg_packet header_comments;
    ogg_packet header_codebooks;
    int result;
    char buf[32];

    vorbis_comment_init (&vc);
    vorbis_comment_add_tag (&vc, "title", text_tag->songname);
    vorbis_comment_add_tag (&vc, "artist", text_tag->artistname);
    vorbis_comment_add_tag (&vc, "album", text_tag->albumname);
    vorbis_comment_add_tag (&vc, "genre", text_tag->genre);
    snprintf (buf, 32, "%d", text_tag->year);
    vorbis_comment_add_tag (&vc, "date", buf);
    snprintf (buf, 32, "%02d", text_tag->track);
    vorbis_comment_add_tag (&vc, "tracknumber", buf);
	
    vorbis_analysis_headerout (&vd, &vc, &header_main, &header_comments, &header_codebooks);

    ogg_stream_packetin (&os, &header_main);
    ogg_stream_packetin (&os, &header_comments);
    ogg_stream_packetin (&os, &header_codebooks);

    while ((result = ogg_stream_flush (&os, &og))) {
      if (result == 0)
	break;
      ret = write_page (&og, out);
      if (ret != og.header_len + og.body_len) {
	log_msg ("Failed writing data to output stream", FL, FN, LN);
	ret = -1;
      }
    }
	  
    sector = cdda_track_firstsector (drive, track);
    last_sector = cdda_track_lastsector (drive, track);
    total_samples_per_channel = (last_sector - sector) * (CD_FRAMESAMPLES / 2);
    int eos = 0;
	
    while (!eos) {
      signed char *buffer = (signed char *)malloc (CD_FRAMESIZE_RAW * READ_SECTORS);
      //use this variable as a slut
      long sectors_read = last_sector - sector;
      if (sectors_read > READ_SECTORS)
	sectors_read = READ_SECTORS;

      sectors_read = cdda_read (drive, (signed char *)buffer, sector, sectors_read);
      int i;
	  
      if (sectors_read == 0) {
	vorbis_analysis_wrote (&vd, 0);
      } else {
	float **vorbbuf = vorbis_analysis_buffer (&vd, CD_FRAMESIZE_RAW * sectors_read);
	for (i = 0; i < (CD_FRAMESIZE_RAW * sectors_read) / 4; i++) {
	  vorbbuf[0][i] = ((buffer[i * 4 + 1] << 8) | (0x00ff&(int)buffer[i * 4])) / 32768.f;
	  vorbbuf[1][i] = ((buffer[i * 4 + 3] << 8) | (0x00ff&(int)buffer[i * 4 + 2])) / 32768.f;
	}

	int samples_read = sectors_read * (CD_FRAMESAMPLES / 2);
	samplesdone += samples_read;
	// progress every 60 pages
	if (packetsdone >= 60) {
	  packetsdone = 0;
	  time = timer_time (timer);
	  update_statistics (total_samples_per_channel, samplesdone, time, track,
			     tracktot, 0, filenames);
	}
	vorbis_analysis_wrote (&vd, i);
      }
	  
      free (buffer);
      sector += sectors_read;
	  
      while (vorbis_analysis_blockout (&vd, &vb) == 1) {
	vorbis_analysis (&vb, &op);
	vorbis_bitrate_addblock (&vb);

	while (vorbis_bitrate_flushpacket (&vd, &op)) {
	  ogg_stream_packetin (&os, &op);
	  packetsdone++;

	  while (!eos) {
	    int result = ogg_stream_pageout (&os, &og);
	    if (result == 0) {
	      break;
	    }
	    ret = write_page (&og, out);
	    if (ret != og.header_len + og.body_len) {
	      log_msg ("Failed writing data to output stream", FL, FN, LN);
	      ret = -1;
	    } else
	      bytes_written += ret;

	    if (ogg_page_eos (&og)) {
	      eos = 1;
	    }
	  }
	}
      }
    }
  }
  ret = 0;

  update_statistics (total_samples_per_channel, samplesdone, time, track,
		     tracktot, 0, filenames);
  
  ogg_stream_clear (&os);
  vorbis_block_clear (&vb);
  vorbis_dsp_clear (&vd);
  vorbis_comment_clear (&vc);
  vorbis_info_clear (&vi);
  vorbis_comment_clear (&vc);
  time_elapsed = timer_time (timer);
  end_func (time_elapsed, rate, samplesdone, bytes_written);
  timer_clear (timer);
  fclose (out);
  
  return ret;
}

static char *build_filename (text_tag_s *text_tag)
{
  size_t size = strlen (text_tag->songname) + 10;
  char *buf = (char *)malloc (size);

  snprintf (buf, size, "%02d - %s.ogg", text_tag->track, text_tag->songname);
  
  return buf;
}

static rip_opts_s *parse_config (PyObject *cfgparser)
{
  PyObject *temp;
  rip_opts_s *rip_opts = (rip_opts_s *)malloc (sizeof (rip_opts_s));
  //rip_opts->device = (char *)malloc (64);
  char *ptr;
  
  rip_opts->read_sectors = 64;
  rip_opts->bitrate = 196;
  rip_opts->min_bitrate = 0;
  rip_opts->max_bitrate = 0;
  rip_opts->managed = 0;
  rip_opts->quality = 1.0;
  rip_opts->quality_set = 0;
  
  srand (time (NULL));

  if (cfgparser == NULL)
    return rip_opts;
  
  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "read_sectors", "RIPPER");
  rip_opts->read_sectors = atoi (PyString_AsString (temp));
  
  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "bitrate", "RIPPER");
  rip_opts->bitrate = atoi (PyString_AsString (temp));
  
  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "min_bitrate", "RIPPER");
  rip_opts->min_bitrate = atoi (PyString_AsString (temp));
  
  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "max_bitrate", "RIPPER");
  rip_opts->max_bitrate = atoi (PyString_AsString (temp));
  
  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "managed", "RIPPER");
  rip_opts->managed = atoi (PyString_AsString (temp));
  
  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "quality", "RIPPER");
  rip_opts->quality = atof (PyString_AsString (temp));
  
  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "quality_set", "RIPPER");
  rip_opts->quality_set = atoi (PyString_AsString (temp));

  temp = PyObject_CallMethod (cfgparser, "get", "s,s", "device", "RIPPER");
  if (temp != NULL) {
    rip_opts->device = strdup (PyString_AsString (temp));
  } else {
    log_msg ("Section \'device\' not found in config file", FL, FN, LN);
    rip_opts->device = strdup ("auto");
  }
  
  return rip_opts;
}

int rip (cdrom_drive *drive, text_tag_s **text_tags, char **filenames)
{
  int i, len;
  rip_opts_s *rip_opts = parse_config (NULL);
  
  for (i = 0; text_tags[i] != NULL; i++)
    /* just counting */;
  len = i; 
  for (i = 0; text_tags[i] != NULL; i++) {
    char *filename;
    if (filenames == NULL) {
      filename = build_filename (text_tags[i]);
    }
    else {
      filename = filenames[i];
    }
    encode_ogg (drive, rip_opts, text_tags[i], i + 1, len, filename, filenames);
    free_text_tag (text_tags[i]);
  }
  free (text_tags);

  update_statistics (0, 0, 0, 0, len, 1, NULL);

  free (rip_opts);
  cdda_close (drive);
  return 0;
}

static void print_rip_opts (rip_opts_s *rip_opts)
{
  printf ("%d\n", rip_opts->read_sectors);
  printf ("%d\n", rip_opts->bitrate);
  printf ("%d\n", rip_opts->min_bitrate);
  printf ("%d\n", rip_opts->max_bitrate);
  printf ("%d\n", rip_opts->managed);
  printf ("%f\n", rip_opts->quality);
  printf ("%d\n", rip_opts->quality_set);
  printf ("%s\n", rip_opts->device);
}

void free_rip_opts (rip_opts_s *rip_opts)
{
  free (rip_opts->device);
  free (rip_opts);
}

PyObject *pyrip_update (void)
{  
  pthread_mutex_lock (&stats_mutex);
  
  PyObject *d, *l;
  int i;

  d = PyDict_New ();

  getTag_helper_int (d, "minutes", stats.minutes);
  getTag_helper_int (d, "seconds", stats.seconds);
  getTag_helper_int (d, "tracknum", stats.tracknum);
  getTag_helper_int (d, "tracktot", stats.tracktot);
  getTag_helper_int (d, "done", stats.done);
  l = filenames_helper_array_string (stats.tracktot, stats.filenames_c);
  getTag_helper_list (d, "filenames_c", l);
  
  if (stats.done == 1) {
    for (i = 0; i < stats.tracktot; i++) {
      free (stats.filenames_c[i]);
    }
    free (stats.filenames_c);
  }

  pthread_mutex_unlock (&stats_mutex);
  return d;
}

typedef struct thread_arg_t {
  char *device;
  char **filenames_c;
  text_tag_s **text_tags;
  rip_opts_s *rip_opts;
  int size;
} thread_arg_s;

void *pyrip_thread (void *arg)
{
  printf ("in thread first\n");

  thread_arg_s *thread_arg = (thread_arg_s *)arg;
  char **filenames_c = thread_arg->filenames_c;
  text_tag_s **text_tags = thread_arg->text_tags;
  rip_opts_s *rip_opts = thread_arg->rip_opts;
  int size = thread_arg->size;
  char *device = thread_arg->device;
  
  cdrom_drive *drive;
  int i;
  
  /* child thread*/  
  printf ("in thread\n");

  printf ("thread_arg: %s\n", thread_arg->device);
  printf ("device: %s\n", device);

  if (strcmp (device, "auto") == 0) {
    drive = cdda_find_a_cdrom (0, NULL);
  } else {
    drive = cdda_identify (device, 0, NULL);
  }
  if (drive == NULL) {
    log_msg ("Couldn't get a CD drive", FL, FN, LN);
    exit (1);
  }
  cdda_open (drive);
  if (filenames_c == NULL) {
    log_msg ("Error allocating filenames_c", FL, FN, LN);
  }
  
  for (i = 0; i < size; i++) {
    print_text_tag (text_tags[i]);
  }

  rip (drive, text_tags, filenames_c);
  
  for (i = 0; i < size; i++) {
    //free_text_tag (text_tags[i]);
    printf ("freeing filename\n");
    free (filenames_c[i]);
  }
  //free (text_tags);
  printf ("freeing filenames\n");
  free (filenames_c);
  printf ("freeing device\n");
  free (device);
  
  printf ("freeing rip opts\n");
  free_rip_opts (rip_opts);
  printf ("freeing thread arg\n");
  free (thread_arg);
  //Py_DECREF (tags);
  //cdda_close (drive);

  printf ("done freeing, detaching\n");

  pthread_detach (pthread_self ());

  printf ("done\n");
  
  return NULL;
}

int pyrip (char *device, PyObject *cfgparser, PyObject *tags, PyObject *filenames)
{
  pthread_t thread = (pthread_t *)malloc (sizeof (pthread_t));
  thread_arg_s *thread_arg;
  PyThreadState *thread_state;
  char **filenames_c;
  text_tag_s **text_tags;
  int size, i;
  PyObject *d, *n;
  rip_opts_s *rip_opts;
  
  thread_arg = (thread_arg_s *)malloc (sizeof (thread_arg));
  rip_opts = parse_config (cfgparser);

  size = PyList_Size (tags);
  
  text_tags = (text_tag_s **)malloc ((sizeof (text_tag_s *) * (size + 1)));
  filenames_c = (char **)malloc ((sizeof (char *) * (size + 1)));
  
  for (i = 0; i < size; i++) {
    printf ("before get item\n");
    d = PyList_GetItem (tags, i);
    
    printf ("Before setTag:\n");
    text_tags[i] = setTag (d);
    //print_text_tag (text_tags[i]);
   
    if (PyInt_Check (filenames)) {
      filenames_c[i] = build_filename (text_tags[i]);
    }
    else {
      n = PyList_GetItem (filenames, i);
      filenames_c[i] = strdup (PyString_AsString (n));
    }
	
    //Py_DECREF (d);
  }
  text_tags[i] = NULL;

  thread_arg->device = strdup (device);
  printf ("device in pyrip is %s\n", device);
  thread_arg->filenames_c = filenames_c;
  thread_arg->text_tags = text_tags;
  thread_arg->rip_opts = rip_opts;
  thread_arg->size = size;

  printf ("creating thread\n");
  
  pthread_create (&thread, NULL, pyrip_thread, thread_arg);
  
  return 0;
}

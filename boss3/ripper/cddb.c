#include "config.h"

#define PYTHON 1

#include <Python.h>

#include <cddb/cddb.h>
#include <cdda_interface.h>

#include "../metadata/id3.h"
#include "cddb.h"

#define VERSION "0.1"
#define NAME "bossogg"

static cddb_conn_t *setup_connection ()
{
  cddb_conn_t *conn = NULL;
  char buf[256];
  
  conn = cddb_new ();
  if (conn == NULL) {
    log_msg ("Out of memory, can't create new connection", FL, FN, LN);
    return;
  }

  cddb_http_enable (conn);
  cddb_set_server_port (conn, 80);
  cddb_set_server_name (conn, "freedb.org");
  cddb_set_http_path_query (conn, "/~cddb/cddb.cgi");
  cddb_set_http_path_submit (conn, "/~cddb/submit.cgi");
  cddb_set_client (conn, NAME, VERSION);
  
  return conn;
}

static void toc_helper (cdrom_drive *drive, cddb_disc_t *disc, int i)
{
  cddb_track_t *track = NULL;
  long sector;
  
  track = cddb_track_new ();
  if (track == NULL) {
    log_msg ("Out of memory, can't create track", FL, FN, LN);
    return;
  }

  sector = cdda_track_firstsector (drive, i);
  cddb_track_set_frame_offset (track, sector);
  cddb_disc_add_track (disc, track);
  
  return;
}

inline static int frames_to_secs (int frames)
{
  return (int)((frames * CD_FRAMESAMPLES) / 44100.0);
}

static cddb_disc_t *init_cddb (cdrom_drive *drive)
{
  int secs, tracks, i, matches;
  cddb_disc_t *disc;
  cddb_conn_t *conn;
  char buf[256];
  
  disc = cddb_disc_new ();
  if (disc == NULL) {
    log_msg ("Out of memory, can't create disc", FL, FN, LN);
    return NULL;
  }

  secs = frames_to_secs (cdda_disc_lastsector (drive) -
			 cdda_disc_firstsector (drive));
  cddb_disc_set_length (disc, secs);
  
  tracks = cdda_tracks (drive);
  for (i = 1; i <= tracks; i++) {
    toc_helper (drive, disc, i);
  }
  if (cddb_disc_calc_discid (disc) == 0) {
    log_msg ("Error calculating disc id", FL, FN, LN);
    cdda_close (drive);
    cddb_disc_destroy (disc);
    return NULL;
  }
  //printf ("Disc ID is: %x\n", cddb_disc_get_discid (disc));

  conn = setup_connection ();
  if (conn == NULL) {
    log_msg ("Problem establishing connection to freedb", FL, FN, LN);
    return NULL;
  }
  matches = cddb_query (conn, disc);
  if (matches <= 0) {
    cddb_disc_destroy (disc);
    return NULL;
  }
  snprintf (buf, 256, "There are %d matches", matches);
  log_msg (buf, FL, FN, LN);

  for (i = 1; i <= matches; i++) {
    int success = cddb_read (conn, disc);
    if (!success)
      log_msg ("Failed to read cddb info", FL, FN, LN);
  }
  return disc;
}

static text_tag_s *cd_get_text_tag (cdrom_drive *drive, cddb_disc_t *disc, int i)
{
  text_tag_s *text_tag = new_text_tag ();
  cddb_track_t *track = cddb_disc_get_track (disc, i);
  if (track == NULL) {
    log_msg ("Problem reading track", FL, FN, LN);
  }
  i = i + 1;

  text_tag->artistname = strdup (cddb_disc_get_artist (disc));
  text_tag->songname = strdup (cddb_track_get_title (track));;
  text_tag->albumname = strdup (cddb_disc_get_title (disc));
  text_tag->genre = strdup (cddb_disc_get_category_str (disc));
  text_tag->year = cddb_disc_get_year (disc);
  text_tag->track = i;
  text_tag->frequency = 44100;
  text_tag->songlength = (double)frames_to_secs (cdda_track_lastsector (drive, i) -
						 cdda_track_firstsector (drive, i));

  return text_tag;
}

text_tag_s **query_cd (cdrom_drive *drive)
{
  text_tag_s **text_tags;
  int tracks, i;
  cddb_disc_t *disc;
  
  if (drive != NULL) {
    tracks = cdda_tracks (drive);

    if (tracks < 0) {
      log_msg ("No tracks found on CD", FL, FN, LN);
      return NULL;
    }
	
    text_tags = (text_tag_s **)malloc (sizeof (text_tag_s) * (tracks + 1));
    disc = init_cddb (drive);
	
    for (i = 0; i < tracks; i++) {
      text_tags[i] = cd_get_text_tag (drive, disc, i);
    }
    text_tags[i] = NULL;

    cddb_disc_destroy (disc);
  } else {
    log_msg ("Couldn't init cd drive", FL, FN, LN);
    return NULL;
  }
  return text_tags;
}

PyObject *getCDDB (char *device)
{
  PyObject *d, *l;
  cdrom_drive *drive;
  char buf[256];
  
  if (strcmp (device, "auto") == 0) {
    drive = cdda_find_a_cdrom (0, NULL);
    if (drive == NULL) {
      log_msg ("Couldn't find a CD drive\n", FL, FN, LN);
      return NULL;
    }
  } else {
    drive = cdda_identify (device, 0, NULL);
    if (drive == NULL) {
      snprintf (buf, 256, "Couldn't identify \'%s\'\n", device);
      log_msg (buf, FL, FN, LN);
      return NULL;
    }
  }
  
  cdda_open (drive);
  text_tag_s **text_tags = query_cd (drive);
  int i, len;

  if (text_tags == NULL) {
    log_msg ("Problem reading CD tags", FL, FN, LN);
    return NULL;
  }

  printf ("cddb tags\n");
  for (i = 0; text_tags[i] != NULL; i++)
    print_text_tag (text_tags[i]);
  /* do nothing, just counting */;
  len = i;
  
  l = PyList_New (len);

  for (i = 0; text_tags[i] != NULL; i++) {
    d = PyDict_New ();
    getTag_helper (d, text_tags[i]);
    PyList_SetItem (l, i, d);
    free_text_tag (text_tags[i]);
  }

  free (text_tags);
  cdda_close (drive);
  return l;
}

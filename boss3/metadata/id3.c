
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define PYTHON 1

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <Python.h>

#ifdef HAVE_VORBIS
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

#ifdef HAVE_FLAC
#ifndef HAVE_VORBIS
#error "FLAC support requires Vorbis support!"
#endif
#include <FLAC/metadata.h>
#endif

#include "id3.h"
#include "id3lib_wrapper.h"

void log_msg (const char *msg, const char *file, const char *function, int line)
{
  printf ("Error: [%s:%s:%d]: %s\n", file, function, line, msg);
  return;
}

text_tag_s *new_text_tag ()
{
  text_tag_s *text_tag = (text_tag_s *)malloc (sizeof (text_tag_s));
  text_tag->artistname = NULL;
  text_tag->songname = NULL;
  text_tag->albumname = NULL;
  text_tag->genre = NULL;
  text_tag->year = 0;
  text_tag->track = 0;
  text_tag->bitrate = -1;
  text_tag->frequency = -1;
  text_tag->songlength = -1.0;
  
  return text_tag;
}

void free_text_tag (text_tag_s *text_tag)
{
  if (text_tag->artistname != NULL) {
	free (text_tag->artistname);
  }
  if (text_tag->songname != NULL) {
	free (text_tag->songname);
  }
  if (text_tag->albumname != NULL) {
	free (text_tag->albumname);
  }
  if (text_tag->genre != NULL) {
	free (text_tag->genre);
  }
  free (text_tag);
}

/* if tag data wasn't available, fill in with "none"
   instead of leaving as NULL */
static void fill_null_text_tag (text_tag_s *text_tag)
{
  char buf[] = "none";
  if (text_tag->artistname == NULL)
	text_tag->artistname = strdup (buf);
  if (text_tag->songname == NULL)
	text_tag->songname = strdup (buf);
  if (text_tag->albumname == NULL)
	text_tag->albumname = strdup (buf);
  if (text_tag->genre == NULL)
	text_tag->genre = strdup (buf);

}

void print_text_tag (text_tag_s *text_tag)
{
  printf ("Artist:    %s\n", text_tag->artistname);
  printf ("Title:     %s\n", text_tag->songname);
  printf ("Album:     %s\n", text_tag->albumname);
  printf ("Genre:     %s\n", text_tag->genre);
  printf ("Year:      %d\n", text_tag->year);
  printf ("Track:     %d\n", text_tag->track);
  printf ("Bitrate:   %d\n", text_tag->bitrate);
  printf ("Frequency: %d\n", text_tag->frequency);
  printf ("Length:    %.2f\n", text_tag->songlength);
}

#ifdef HAVE_ID3
/* mp3 tags are handled in id3lib_wrapper.cc */
static text_tag_s *get_mp3_text_tag (char *filename)
{
  text_tag_s *text_tag = new_text_tag ();

  if (id3lib_wrapper (filename, &text_tag) < 0) {
	free_text_tag (text_tag);
	return NULL;
  }
  fill_null_text_tag (text_tag);
  
  return text_tag;
}
#endif

#ifdef HAVE_VORBIS
/* parse ogg comment, return type */
static int get_ogg_type (char *str)
{
  char *ptr = strstr (str, "=");
  ptr = ptr + 1;
  int len = ptr - str;
  int ret;
  if (strncasecmp (str, "artist=", len) == 0)
	return ARTIST;
  if (strncasecmp (str, "title=", len) == 0)
	return TITLE;
  if (strncasecmp (str, "album=", len) == 0)
	return ALBUM;
  if (strncasecmp (str, "genre=", len) == 0)
	return GENRE;
  if (strncasecmp (str, "date=", len) == 0)
	return YEAR;
  if (strncasecmp (str, "tracknumber=", len) == 0)
	return TRACK;

  return -1;
}
#endif

#ifdef HAVE_VORBIS
/* fill in the appropriate text_tag_s field with str */
static int parse_ogg_content (char *str, int length, text_tag_s **text_tag, int type)
{
  //printf ("passed length of %d\n", length);
  char *ptr = strstr (str, "=");
  ptr = ptr + 1;
  //int len = strlen (ptr) + 1;
  int len = length - (ptr - str);
  //printf ("strlen is %d: %s\n", len, str);
  switch (type) {
  case ARTIST:
    (*text_tag)->artistname = (char *)malloc (len + 1);
    strncpy ((*text_tag)->artistname, ptr, len);
    (*text_tag)->artistname[len] = '\0';
    break;
  case TITLE:
    (*text_tag)->songname = (char *)malloc (len + 1);
    strncpy ((*text_tag)->songname, ptr, len);
    (*text_tag)->songname[len] = '\0';
    break;
  case ALBUM:
    (*text_tag)->albumname = (char *)malloc (len + 1);
    strncpy ((*text_tag)->albumname, ptr, len);
    (*text_tag)->albumname[len] = '\0';
    break;
  case GENRE:
    (*text_tag)->genre = (char *)malloc (len + 1);
    strncpy ((*text_tag)->genre, ptr, len);
    (*text_tag)->genre[len] = '\0';
    break;
  case YEAR:
    (*text_tag)->year = atoi (ptr);
    break;
  case TRACK:
    (*text_tag)->track = atoi (ptr);
    break;
  default:
    //printf ("unhandled type %d\n", type);
    break;
  }
  //printf ("parse ogg got: \'%s\'\n", ptr);

  return 1;
}
#endif

#ifdef HAVE_FLAC
/* FLAC implementation. notice, it uses the ogg functions
   when the FLAC comment is extracted */
static text_tag_s *get_flac_text_tag (const char *filename)
{
  text_tag_s *text_tag = new_text_tag ();
  char buf[256];
  FLAC__StreamMetadata *metadata = malloc (sizeof (FLAC__StreamMetadata));
  FLAC__Metadata_SimpleIterator *iter = FLAC__metadata_simple_iterator_new ();
  if (FLAC__metadata_simple_iterator_init (iter, filename, true, false) != true) {
	snprintf (buf, 256, "Problem reading file \'%s\'", filename);
	log_msg (buf, FL, FN, LN);
	free (metadata);
	FLAC__metadata_simple_iterator_delete (iter);
	free_text_tag (text_tag);
	return NULL;
  }

  do {
	int i = 0;
	
	switch (FLAC__metadata_simple_iterator_get_block_type (iter)) {
	case FLAC__METADATA_TYPE_STREAMINFO:
	  metadata = FLAC__metadata_simple_iterator_get_block (iter);
	  
	  text_tag->frequency = metadata->data.stream_info.sample_rate;
	  text_tag->songlength = metadata->data.stream_info.total_samples /
		metadata->data.stream_info.sample_rate;
	  text_tag->bitrate = (int)((metadata->data.stream_info.total_samples *
								 metadata->data.stream_info.bits_per_sample) / text_tag->songlength);
	  break;
	case FLAC__METADATA_TYPE_VORBIS_COMMENT:
	  metadata = FLAC__metadata_simple_iterator_get_block (iter);

	  for (i = 0; i < metadata->data.vorbis_comment.num_comments; i++) {
		int type = get_ogg_type (metadata->data.vorbis_comment.comments[i].entry);
		parse_ogg_content (metadata->data.vorbis_comment.comments[i].entry, metadata->data.vorbis_comment.comments[i].length, &text_tag, type);
	  }
	  
	  break;
	default:
	  //printf ("Unhandled metadata type\n");
	  break;
	}
  } while (FLAC__metadata_simple_iterator_next (iter) == true);

  fill_null_text_tag (text_tag);
  
  FLAC__metadata_simple_iterator_delete (iter);
  free (metadata);
		   
  return text_tag;
}
#endif

#ifdef HAVE_VORBIS
/* ogg implementation. */
static text_tag_s *get_ogg_text_tag (const char *filename)
{
  text_tag_s *text_tag = new_text_tag ();
  char buf[256];
  FILE *fd = fopen (filename, "r");
  if (fd == NULL) {
	snprintf (buf, 256, "Problem opening file \'%s\'", filename);
	log_msg (buf, FL, FN, LN);
	free_text_tag (text_tag);
	return NULL;
  }
  OggVorbis_File vf;
  vorbis_info *vi;
  vorbis_comment *vc;
  char **str;

  if (ov_open (fd, &vf, NULL, 0) < 0) {
	snprintf (buf, 256, "Problem reading file \'%s\'", filename);
	log_msg (buf, FL, FN, LN);
	free_text_tag (text_tag);
	fclose (fd);
	return NULL;
  }

  vc = ov_comment (&vf, -1);
  vi = ov_info (&vf, -1);
  text_tag->bitrate = ov_bitrate (&vf, -1);
  text_tag->songlength = ov_time_total (&vf, -1);
  text_tag->frequency =  vi->rate;
  str = vc->user_comments;
  int length_i = 0;
  while (*str) {
	int type = get_ogg_type (*str);
	if (type != -1)
	  parse_ogg_content (*str, vc->comment_lengths[length_i], &text_tag, type);
	++str;
	length_i++;
  }

  fill_null_text_tag (text_tag);
  
  ov_clear (&vf);
  return text_tag;
}
#endif

/* simple file type determination based on extension */
static int determine_file_type (const char *filename)
{
  int len = strlen (filename);

  if (strncmp (&filename[len - 3], "mp3", 3) == 0) {
	return MP3_FILE;
  } else if (strncmp (&filename[len  - 3], "ogg", 3) == 0) {
	return OGG_FILE;
  } else if (strncmp (&filename[len - 4], "flac", 4) == 0) {
	return FLAC_FILE;
  } else
	log_msg ("Unhandled file type", FL, FN, LN);

  return -1;
}

/* determine file type and pass it along to the proper
   helper function */
text_tag_s *get_text_tag (char *filename)
{
  if (filename == NULL)
	return NULL;
  int file_type = determine_file_type (filename);
  text_tag_s *text_tag = NULL;

  if (file_type == MP3_FILE) {
#ifdef HAVE_ID3
	text_tag = get_mp3_text_tag (filename);
#else
	log_msg ("Not compiled with MP3 support", FL, FN, LN);
#endif
  } else if (file_type == OGG_FILE) {
#ifdef HAVE_VORBIS
	text_tag = get_ogg_text_tag (filename);
#else
	log_msg ("Not compiled with Vorbis support", FL, FN, LN);
#endif
  } else if (file_type == FLAC_FILE) {
#ifdef HAVE_FLAC
	text_tag = get_flac_text_tag (filename);
#else
	log_msg ("Not compiled with FLAC support", FL, FN, LN);
#endif
  } else {
	return NULL;
  }

  return text_tag;
}

void getTag_helper_string (PyObject *d, char *key, char *str)
{
  PyObject *obj;

  obj = PyString_FromString (str);
  PyDict_SetItemString (d, key, obj);
  Py_DECREF (obj);
}

void getTag_helper_list (PyObject *d, char *key, PyObject *l)
{
  PyDict_SetItemString (d, key, l);
}

PyObject *filenames_helper_array_string (int size, char **str)
{
  PyObject *obj, *l;
  int i, len;
  
  l = PyList_New (size);
  
  for (i = 0; i < size; i++) {
    obj = PyString_FromString (str[i]);
    PyList_SetItem (l, i, obj);
    //Py_DECREF (obj);
  }
  
  return l;
}

void setTag_helper_string (PyObject *d, char *key, char **str)
{
  PyObject *obj;

  obj = PyDict_GetItemString (d, key);
  *str = strdup (PyString_AsString (obj));
  //Py_DECREF (obj);
}

void getTag_helper_int (PyObject *d, char *key, int num)
{
  PyObject *obj;

  obj = PyInt_FromLong (num);
  PyDict_SetItem (d, PyString_FromString (key), obj);
  Py_DECREF (obj);
}

void setTag_helper_int (PyObject *d, char *key, int *num)
{
  PyObject *obj;

  obj = PyDict_GetItemString (d, key);
  *num = PyInt_AsLong (obj);
  //Py_DECREF (obj);
}

void getTag_helper_double (PyObject *d, char *key, double num)
{
  PyObject *obj;

  obj = PyFloat_FromDouble (num);
  PyDict_SetItem (d, PyString_FromString (key), obj);
  Py_DECREF (obj);
}

void setTag_helper_double (PyObject *d, char *key, double *num)
{
  PyObject *obj;
  obj = PyDict_GetItemString (d, key);
  *num = PyFloat_AsDouble (obj);
  //Py_DECREF (obj);
}

void getTag_helper (PyObject *d, text_tag_s *text_tag)
{
  getTag_helper_string (d, "artistname", text_tag->artistname);
  getTag_helper_string (d, "songname", text_tag->songname);
  getTag_helper_string (d, "albumname", text_tag->albumname);
  getTag_helper_string (d, "genre", text_tag->genre);
  getTag_helper_int (d, "year", text_tag->year);
  getTag_helper_int (d, "tracknum", text_tag->track);
  getTag_helper_int (d, "bitrate", text_tag->bitrate);
  getTag_helper_int (d, "frequency", text_tag->frequency);
  getTag_helper_double (d, "songlength", text_tag->songlength);

  return;
}

void setTag_helper (PyObject *d, text_tag_s *text_tag)
{
  setTag_helper_string (d, "artistname", &text_tag->artistname);
  setTag_helper_string (d, "songname", &text_tag->songname);
  setTag_helper_string (d, "albumname", &text_tag->albumname);
  setTag_helper_string (d, "genre", &text_tag->genre);
  setTag_helper_int (d, "year", &text_tag->year);
  setTag_helper_int (d, "tracknum", &text_tag->track);
  setTag_helper_int (d, "bitrate", &text_tag->bitrate);
  setTag_helper_int (d, "frequency", &text_tag->frequency);
  setTag_helper_double (d, "songlength", &text_tag->songlength);

  return;
}

text_tag_s *setTag (PyObject *tag)
{
  text_tag_s *text_tag = new_text_tag ();

  setTag_helper (tag, text_tag);
  
  return text_tag;
}

PyObject *getTag (char *filename)
{
  PyObject *d;
  text_tag_s *text_tag;
  
  d = PyDict_New ();
  text_tag = get_text_tag (filename);

  if (text_tag == NULL) {
	PyObject *obj = PyString_FromString ("Problem reading file header");
	PyDict_SetItemString (d, "songname", obj);
	return d;
  }
  
  getTag_helper (d, text_tag);
  
  free_text_tag (text_tag);
  return d;
}

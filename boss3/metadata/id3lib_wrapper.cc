
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ID3
//#include <id3.h>
#include <id3/globals.h>
#include <id3/tag.h>
#include <id3/misc_support.h>
using namespace std;
//using namespace id3;
#endif

#include <stdio.h> //For printf
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "id3.h"
#include "id3lib_wrapper.h"

#include <Python.h>
#include <ao/ao.h>
#include <mad.h>
#include <bossao.h>
#include <mp3.h>

/* libid3 is a c++ library, so we have to wrap it here to
   use it from C */
#ifdef HAVE_MP3
extern "C" {
static void mp3_init (mp3_s *mp3)
{
  mp3->out_ptr = mp3->out_buf;
  mp3->out_buf_end = mp3->out_buf + BUF_SIZE;
  mp3->frame_count = 0;
  mp3->status = 0;
  mp3->start = 0;
  mad_stream_init (&mp3->stream);
  mad_frame_init (&mp3->frame);
  mad_synth_init (&mp3->synth);
  mad_timer_reset (&mp3->timer);
}

static void mp3_finish (mp3_s *mp3)
{
  mad_synth_finish (&mp3->synth);
  mad_frame_finish (&mp3->frame);
  mad_stream_finish (&mp3->stream);

  fclose (mp3->file);
}

static int mp3_fill_input (mp3_s *mp3)
{
  size_t size, remaining;
  unsigned char *read_start;

  if (mp3->stream.next_frame != NULL) {
    remaining = mp3->stream.bufend - mp3->stream.next_frame;
    memmove (mp3->read_buf, mp3->stream.next_frame, remaining);
    read_start = mp3->read_buf + remaining;
    size = READ_BUFFER_SIZE - remaining;
  } else {
    size = READ_BUFFER_SIZE;
    read_start = mp3->read_buf;
    remaining = 0;
  }

  size = fread (read_start, 1, size, mp3->file);
  if (size <= 0)
    return -1;

  mad_stream_buffer (&mp3->stream, mp3->read_buf, size + remaining);
  mp3->stream.error = (mad_error)0;

  return 0;
}

static int mp3_decode_frame (mp3_s *mp3)
{
  if (mp3->stream.buffer == NULL || mp3->stream.error == MAD_ERROR_BUFLEN) {
    if (mp3_fill_input (mp3) < 0) {
      return DECODE_BREAK;
    }
  }

  if (mad_frame_decode (&mp3->frame, &mp3->stream)) {
    if (MAD_RECOVERABLE (mp3->stream.error)) {
      return DECODE_CONT;
    } else {
      if (mp3->stream.error == MAD_ERROR_BUFLEN) {
	return DECODE_CONT;
      } else {
	printf ("Unrecoverable frame error: \'%s\'\n", mad_stream_errorstr (&mp3->stream));
	mp3->status = 1;
	return DECODE_BREAK;
      }
    }
  }

  mp3->frame_count++;

  return DECODE_OK;
}

static int mp3_open (mp3_s *mp3, char *filename)
{
  int ret;
  struct stat filestat;

  mp3_init (mp3);
  if ((mp3->file = fopen (filename, "r")) <= 0) {
    printf ("Problem opening file \'%s\'\n", filename);
    return -1;
  }
  fstat (fileno (mp3->file), &filestat);
  while ((ret = mp3_decode_frame (mp3)) == DECODE_CONT)
    ; //nothing
  mp3->total_time = (filestat.st_size * 8.0) / mp3->frame.header.bitrate;

  return ret;
}
}
#endif
#ifdef HAVE_ID3
/* create a tag, link it to given filename */
static ID3_Tag *prepare_tag (char *filename)
{
  char buf[256];
  ID3_Tag *tag = new ID3_Tag;
  size_t ret;

  struct stat stat_s;
  ret = stat (filename, &stat_s);
  if (ret == -1) {
    snprintf (buf, 256, "Couldn't stat \'%s\'", filename);
    log_msg (buf, FL, FN, LN);
    return NULL;
  }

  ret = tag->Link (filename, ID3TT_ALL);
  if (ret == 0) {
    //tag->Update (ID3TT_ALL);
    snprintf (buf, 256, "Problem reading file \'%s\', trying a different way", filename);
    log_msg (buf, FL, FN, LN);
    delete tag;
    tag = new ID3_Tag;
    printf ("trying to link: \'%s\'\n", filename);
    ret = tag->Link (filename, true, true);
    if (ret == 0) {
      snprintf (buf, 256, "Sorry, I tried to update but the tag is still messed up");
      log_msg (buf, FL, FN, LN);
    }
    //delete tag;
    //return NULL;
  } else {
    snprintf (buf, 256, "No problem at all");
    log_msg (buf, FL, FN, LN);
  }
  tag->SetUnsync (false);
  tag->SetExtendedHeader (true);
  tag->SetCompression (true);
  tag->SetPadding (true);

  return tag;
}

/* fill in text_tag_s with mp3 header data
   (bitrate, frequency, length) */
static int get_header_info (ID3_Tag *tag, text_tag_s **text_tag)
{
  const Mp3_Headerinfo *mp3info = tag->GetMp3HeaderInfo ();
  char buf[256];

  if (mp3info == NULL) {
    snprintf (buf, 256, "Using workaround for some ID3V1 tags");
    log_msg (buf, FL, FN, LN);

    mp3_s *mp3 = (mp3_s *)malloc (sizeof (mp3_s));
    mp3_init (mp3);
    mp3_open (mp3, (char *)tag->GetFileName ());

    (*text_tag)->bitrate = mp3->frame.header.bitrate;
    (*text_tag)->frequency = 44100;
    (*text_tag)->songlength = mp3->total_time;

    mp3_finish (mp3);

    return 0;
  }
  
  (*text_tag)->bitrate = mp3info->bitrate;
  (*text_tag)->frequency = mp3info->frequency;
  if (mp3info->time == 0) {
    snprintf (buf, 256, "Working around an empty size field");
    log_msg (buf, FL, FN, LN);
    (*text_tag)->songlength = (double)(tag->GetFileSize () * 8) / (double)mp3info->bitrate;
  } else {
    (*text_tag)->songlength = (double)mp3info->time;
  }

  return 0;
}

/* return a string describing the libid3 constants we are using */
static const char *frame_id_text (ID3_FrameID fid)
{  
  if (fid == ID3FID_LEADARTIST)
    return "artistname";
  if (fid == ID3FID_TITLE)
    return "songname";
  if (fid == ID3FID_ALBUM)
    return "albumname";
  if (fid == ID3FID_YEAR)
    return "year";
  if (fid == ID3FID_TRACKNUM)
    return "track";
  if (fid == ID3FID_CONTENTGROUP)
    return "genre";
}

/* i guess with mp3 genre isn't stored in the normal tag
   field either. let's grab that genre */
static void get_genre (ID3_Tag *tag, text_tag_s **text_tag)
{
  (*text_tag)->genre = ID3_GetGenre (tag);
}

/* fill in a text_tag with an mp3 tag field, based on type */
static void fill_text_tag (ID3_Tag *tag, text_tag_s **text_tag, int type)
{
  ID3_FrameID fid;
  char *ptr;
  char buf[256];

  if (type == ARTIST) {
    fid = ID3FID_LEADARTIST;
    (*text_tag)->artistname = ID3_GetArtist (tag);
  } else if (type == TITLE) {
    fid = ID3FID_TITLE;
    (*text_tag)->songname = ID3_GetTitle (tag);
  } else if (type == ALBUM) {
    fid = ID3FID_ALBUM;
    (*text_tag)->albumname = ID3_GetAlbum (tag);
  } else if (type == YEAR) {
    fid = ID3FID_YEAR;
    ptr = ID3_GetYear (tag);
    if (ptr != NULL)
      (*text_tag)->year = atoi (ptr);
  } else if (type == TRACK) {
    fid = ID3FID_TRACKNUM;
    ptr = ID3_GetTrack (tag);
    if (ptr != NULL)
      (*text_tag)->track = atoi (ptr);
  } else if (type == GENRE) {
    fid = ID3FID_CONTENTGROUP;
    (*text_tag)->genre = ID3_GetGenre (tag);
  } else {
    snprintf (buf, 256, "Type \'%s\' not found in frame", frame_id_text (fid));
    log_msg (buf, FL, FN, LN);
  }
}

/* the main wrapper function */
int id3lib_wrapper (char *filename, text_tag_s **text_tag)
{
  ID3_Tag *tag = prepare_tag (filename);
  if (tag == NULL)
    return -1;

  if (get_header_info (tag, text_tag) < 0) {
    log_msg ("Couldn't read mp3 header", FL, FN, LN);
    delete tag;
    return -1;
  }
  fill_text_tag (tag, text_tag, ARTIST);
  fill_text_tag (tag, text_tag, TITLE);
  fill_text_tag (tag, text_tag, ALBUM);
  fill_text_tag (tag, text_tag, YEAR);
  fill_text_tag (tag, text_tag, TRACK);
  fill_text_tag (tag, text_tag, GENRE);
  
  delete tag;
  
  return 0;
}
#endif

/*
 * Boss Ogg - A Music Server
 * (c)2003 by Adam Torgerson (may1937@users.sf.net)
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Python.h>

#include <FLAC/file_decoder.h>
#include <FLAC/metadata.h>
#include <ao/ao.h>
#include <pthread.h>

#include <config.h>

#include "bossao.h"
#include "flac.h"

static FLAC__bool eof_callback (const FLAC__SeekableStreamDecoder *decoder, void *client_data)
{
  //printf ("FLAC got eof\n");
}

static FLAC__StreamDecoderWriteStatus write_callback (const FLAC__FileDecoder *decoder,
						      const FLAC__Frame *frame,
						      const FLAC__int32 *const buffer[],
						      void *client_data)
{
  //printf ("in write\n");
  size_t size = frame->header.blocksize * frame->header.channels;
  size_t samples = frame->header.blocksize;
  uint_16 buf[samples*frame->header.channels];
  int c_samp, c_chan, d_samp;
  unsigned char *bufc = (unsigned char *)buf;
  song_s *song = (song_s *)client_data;

  for (c_samp = d_samp = 0; c_samp < frame->header.blocksize; c_samp++) {
    for (c_chan = 0; c_chan < frame->header.channels; c_chan++, d_samp++) {
      buf[d_samp] = buffer[c_chan][c_samp];
    }
  }
#ifdef WORDS_BIGENDIAN
  unsigned char *buf_pos = bufc;
  unsigned char *buf_end = bufc + sizeof (buf);
  while (buf_pos < buf_end) {
    unsigned char p = *buf_pos;
    *buf_pos = *(buf_pos + 1);
    *(buf_pos + 1) = p;
    buf_pos += 2;
  }
#endif

  bossao_play_chunk (song, bufc, sizeof (buf));

  song->curtime += ((double)samples) / frame->header.sample_rate;


  //printf ("in the write callback, size is %d\n", sizeof (buf));

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void metadata_callback (const FLAC__FileDecoder *decoder,
				const FLAC__StreamMetadata *metadata,
				void *client_data)
{
  //printf ("Doing nothing in metadata callback\n");
}

static void error_callback (const FLAC__FileDecoder *decoder,
			    const FLAC__StreamDecoderErrorStatus status,
			    void *client_data)
{
  printf ("A FLAC error occured\n");
 }

void *prepare_flac (song_s *song, char *filename)
{
  //song->songlib->flac = malloc (sizeof (FLAC__FileDecoder));
  (FLAC__FileDecoder *)song->songlib->flac = FLAC__file_decoder_new ();
  FLAC__FileDecoder *decoder = (FLAC__FileDecoder *)song->songlib->flac;
  
  /* callbacks here? */
  FLAC__file_decoder_set_write_callback (decoder, write_callback);
  FLAC__file_decoder_set_metadata_callback (decoder, metadata_callback);
  FLAC__file_decoder_set_error_callback (decoder, error_callback);
  FLAC__file_decoder_set_client_data (decoder, song);
  //printf ("set callbacks\n");
  
  FLAC__file_decoder_set_filename (decoder, filename);

  FLAC__SeekableStreamDecoderState state = FLAC__file_decoder_init (decoder);
  //printf ("inited\n");
  if (state != FLAC__FILE_DECODER_OK) {
    printf ("Problem initizlizing FLAC file decoder: ", state);
    if (state == FLAC__FILE_DECODER_ALREADY_INITIALIZED)
      printf ("already inited\n");
    if (state == FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR)
      printf ("seekable decoder error\n");
    return NULL;
  }

  FLAC__Metadata_SimpleIterator *it = FLAC__metadata_simple_iterator_new ();
  if (!FLAC__metadata_simple_iterator_init (it, filename, 1, 0)) {
    FLAC__metadata_simple_iterator_delete (it);
    return NULL;
  }
  FLAC__StreamMetadata *block = NULL;
  do {
    if (block)
      FLAC__metadata_object_delete (block);
    block = FLAC__metadata_simple_iterator_get_block (it);
    if (block->type == FLAC__METADATA_TYPE_STREAMINFO)
      break;
  } while (FLAC__metadata_simple_iterator_next (it));
  song->length = ((double)block->data.stream_info.total_samples) / block->data.stream_info.sample_rate;
  song->curtime = 0;
  //printf ("got a length of %f\n", song->length);

  FLAC__metadata_object_delete (block);
  FLAC__metadata_simple_iterator_delete (it);

  //printf ("returning\n");
  return decoder;
}

int destroy_flac (song_s *song)
{
  FLAC__FileDecoder *decoder = (FLAC__FileDecoder *)song->songlib->flac;

  if (decoder != NULL) {
    FLAC__file_decoder_finish (decoder);
    /* seperate this somehow? */
    FLAC__file_decoder_delete (decoder);
    //free (decoder);
    decoder = NULL;
  }

  return 0;
}

long chunk_play_flac (song_s *song, char *buffer)
{
  FLAC__FileDecoder *decoder = (FLAC__FileDecoder *)song->songlib->flac;
  //printf ("trying to play a chunk %x\n", decoder);
  FLAC__file_decoder_process_single (decoder);
  //printf ("done chunking\n");

  if (FLAC__file_decoder_get_state (decoder) == FLAC__FILE_DECODER_END_OF_FILE) {
    //printf ("flac reached end of file\n");
    return 0;
  }

  return 1;
}

int seek_flac (song_s *song)
{
  printf ("Seeking not yet implemented for FLAC\n");

  return -1;
}

double time_total_flac (song_s *song)
{
  return song->length;
}

double time_current_flac (song_s *song)
{
  return song->curtime;
}

int identify_flac (song_s *song, FILE *file)
{
  int ret = -1;

  int len = strlen (song->filename);
  char *ptr = &song->filename[len - 4];

  if (strncmp (ptr, "flac", 4) == 0)
    ret = 0;

  return ret;
}

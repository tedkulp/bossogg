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
#include <vorbis/vorbisfile.h>
#include <ao/ao.h>
#include <Python.h>

#include "../../config.h"
#include "bossao.h"
#include "ogg.h"

OggVorbis_File *prepare_ogg (song_s *song, char *filename)
{
  vorbis_info *vi = NULL;
  FILE *fp;
  
  song->songlib->vf = (OggVorbis_File *)malloc (sizeof (OggVorbis_File));
  OggVorbis_File *vf = song->songlib->vf;
  
  fp = fopen (filename, "r");
  if (fp == NULL) {
    printf ("Error opening \'%s\'\n", filename);
    free (vf);
    return NULL;
  }
  if (ov_open (fp, vf, NULL, 0) < 0) {
    printf ("File \'%s\' does not appear to be an Ogg file\n", filename);
    ov_clear (vf);
    free (vf);
    return NULL;
  }
  
  vi = ov_info (vf, -1);
  if (vi->channels != 2) {
    printf ("%d channels is not currently supported\n", vi->channels);
    ov_clear (vf);
    free (vf);
    return NULL;
  }
  if (vi->rate != 44100) {
    printf ("A rate of %d is not currently supported\n", vi->rate);
    ov_clear (vf);
    free (vf);
    return NULL;
  }
  
  return vf;
}

int destroy_ogg (song_s *song)
{
  ov_clear (song->songlib->vf);
  free (song->songlib->vf);
	
  return 0;	
}

long chunk_play_ogg (song_s *song, char *buffer)
{
  int ret;
  long bytes_read;
  int current;

  bytes_read = ov_read (song->songlib->vf, buffer, BUF_SIZE, 0, 2, 1, &current);
  bossao_play_chunk (song, buffer, bytes_read);

  return bytes_read;
}

int seek_ogg (song_s *song)
{
  ov_time_seek(song->songlib->vf, song->seek);
  return 1;
}

double time_total_ogg (song_s *song)
{
  return ov_time_total (song->songlib->vf, -1);
}

double time_current_ogg (song_s *song)
{
  return ov_time_tell (song->songlib->vf);
}

int identify_ogg (song_s *song, FILE *file)
{
  int ret;
  song->songlib->vf = (OggVorbis_File *)malloc (sizeof (OggVorbis_File));
  ret = ov_test (file, song->songlib->vf, NULL, 0);
  ov_clear (song->songlib->vf);
  free (song->songlib->vf);

  return ret;
}

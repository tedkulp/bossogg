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

//#include "bossao.h"

#define READ_BUFFER_SIZE (4*8192)

#define DECODE_BREAK -2
#define DECODE_CONT -1
#define DECODE_OK 0

typedef struct mp3_t {
  FILE *file;
  struct mad_stream stream;
  struct mad_frame frame;
  struct mad_synth synth;
  mad_timer_t timer;
  unsigned char read_buf[READ_BUFFER_SIZE];
  char out_buf[BUF_SIZE];
  char *out_ptr;
  char *out_buf_end;
  unsigned long frame_count;
  int status;
  float total_time;
  float elapsed_time;
  int start;
} mp3_s;

#ifdef __cplusplus
extern "C" {
#endif

void *prepare_mp3 (song_s *song, char *filename);
int destroy_mp3 (song_s *song);
char *chunk_play_mp3 (song_s *song, int *size);
int seek_mp3 (song_s *song);
double time_total_mp3 (song_s *song);
double time_current_mp3 (song_s *song);
int identify_mp3 (song_s *song, FILE *file);

#ifdef __cplusplus
}
#endif

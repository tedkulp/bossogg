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

OggVorbis_File *prepare_ogg (song_s *song, char *filename);
int destroy_ogg (song_s *song);
long chunk_play_ogg (song_s *song, char *buffer);
int seek_ogg (song_s *song);
double time_total_ogg (song_s *song);
double time_current_ogg (song_s *song);
int identify_ogg (song_s *song, FILE *file);

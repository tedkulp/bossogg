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
*/

#import <glib.h>

typedef struct {
   GMutex *mutex;
   GCond *cond;
   gint count;
} semaphore_t;

typedef struct {
   void **buf;
   size_t *chunk_size;
   //size_t head, tail;
   //size_t size;
   //int full, empty, paused;
   //int readers, writers, waiting;
   GMutex *mutex;
   semaphore_t *empty, *full;
} thbuf_t;

#define THBUF_SIZE 256

semaphore_t *semaphore_new (int count);
void semaphore_free (semaphore_t *semaphore);
int semaphore_p (semaphore_t *semaphore);
int semaphore_v (semaphore_t *sempahore);

int thbuf_produce (thbuf_t *buf, void *p, size_t size, int pos);
void *thbuf_consume (thbuf_t *buf, size_t *size, int pos);
void thbuf_clear (thbuf_t *buf);

//void thbuf_read_lock (thbuf_s *buf, int d);
//void thbuf_read_unlock (thbuf_s *buf);
//void thbuf_write_lock (thbuf_s *buf, int d);
//void thbuf_write_unlock (thbuf_s *buf);

thbuf_t *thbuf_new (size_t size);
void thbuf_free (thbuf_t *buf);


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
} thbuf_sem_t;

typedef struct chunk_t {
   guchar *chunk;
   gint sample_num;
   gint size;
   gchar eof;
   //GMutex *mutex;
} chunk_s;

typedef void (*thbuf_free_callback_t)(void *p);


typedef struct thbuf_ {
   gint size;
   gint produce_pos;
   gint consume_pos;
   void **buf;
   GMutex *mutex;
   thbuf_sem_t *empty, *full;
   thbuf_free_callback_t free_cb;
} thbuf_t;

#define THBUF_SIZE 256

thbuf_sem_t *semaphore_new (int count);
void semaphore_free (thbuf_sem_t *semaphore);
gint semaphore_p (thbuf_sem_t *semaphore);
gint semaphore_v (thbuf_sem_t *sempahore);

gint thbuf_produce (thbuf_t *buf, void *p);
void *thbuf_consume (thbuf_t *buf, gint *count);
gint thbuf_current_size (thbuf_t *buf);
void thbuf_clear (thbuf_t *buf);
void thbuf_set_free_callback (thbuf_t *thbuf, thbuf_free_callback_t cb);

thbuf_t *thbuf_new (size_t size);
void thbuf_free (thbuf_t *buf);


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

 *************************************************************************
 * threaded buffer implementation (pretty speedy)
 *
 * pretty basic abstractions around mutexes/conds to form counted
 *  semaphores. then an abstraction around that for a producer/consumer
 *  threaded buffer.
 *************************************************************************
 
*/

#import <glib.h>

#import <stdio.h>
#import <stdlib.h>

#include "thbuf.h"
#include "common.h"

/* allocate a new semaphore
   mutex is the pre-allocated mutex to associate with the semaphore */
thbuf_sem_t *semaphore_new (gint count)
{
   thbuf_sem_t *sem = (thbuf_sem_t *)g_malloc (sizeof (thbuf_sem_t));
   sem->mutex = g_mutex_new ();
   sem->cond = g_cond_new ();

   sem->count = count;

   return sem;
}

/* free a semaphore
   the mutex associated with it is NOT freed */
void semaphore_free (thbuf_sem_t *sem)
{
   g_cond_free (sem->cond);

   g_free (sem);
}

/* performs a p operation on the semaphore
   returns the count of the semaphore */
gint semaphore_p (thbuf_sem_t *sem)
{
   // wait on the count, decrement it 
   g_mutex_lock (sem->mutex);
   while (sem->count < 1) {
      g_cond_wait (sem->cond, sem->mutex);
   }
   sem->count--;
   g_mutex_unlock (sem->mutex);

   return sem->count;
}

/* performs a v operation on the semaphore
   returns the count of the semaphore */
gint semaphore_v (thbuf_sem_t *sem)
{
   // increment the count, signal the others 
   g_mutex_lock (sem->mutex);
   sem->count++;
   g_cond_broadcast (sem->cond);
   g_mutex_unlock (sem->mutex);

   return sem->count;
}

/* production function
   adds p the the thbuf
   adds size to the size array */
gint thbuf_produce (thbuf_t *buf, void *p)
{
   // perform a p operation on empty, makes less empty 
   semaphore_p (buf->empty);

   // critical section, add the data to the thbuf 
   g_mutex_lock (buf->mutex);
   buf->buf[buf->produce_pos] = p;
   buf->produce_pos = (buf->produce_pos + 1) % THBUF_SIZE;
   //LOG ("added %p size %d to %d", buf->buf[pos], buf->chunk_size[pos], pos);
   g_mutex_unlock (buf->mutex);

   // perform a v operation on full, makes more full 
   semaphore_v (buf->full);  
   
   return buf->full->count;
}

/* consumption function
   returns p that was added
   size is returned to be the size associated with p */
void *thbuf_consume (thbuf_t *buf, gint *count)
{
   // perform a p operation on full, makes less full 
   semaphore_p (buf->full);

   //critical section, remove the data from the thbuf 
   g_mutex_lock (buf->mutex);
   void *ret = buf->buf[buf->consume_pos];
   buf->buf[buf->consume_pos] = NULL;
   if (count != NULL)
      *count = buf->full->count;
   buf->consume_pos = (buf->consume_pos + 1) % THBUF_SIZE;
   //LOG ("got %p from %d e:%d f:%d %d", ret, pos, buf->empty->count, buf->full->count, *size);
   g_mutex_unlock (buf->mutex);

   // perform a v operation on empty, makes more empty
   semaphore_v (buf->empty);

   return ret;
}

/* consumption function, non locking version
   returns p that was added
   size is returned to be the size associated with p */
static void *thbuf_consume_no_lock (thbuf_t *buf, gint *count)
{
   // perform a p operation on full, makes less full 
   semaphore_p (buf->full);

   //critical section, remove the data from the thbuf 
   void *ret = buf->buf[buf->consume_pos];
   buf->buf[buf->consume_pos] = NULL;
   if (count != NULL)
      *count = buf->full->count;
   buf->consume_pos = (buf->consume_pos + 1) % THBUF_SIZE;
   //LOG ("got %p from %d e:%d f:%d %d", ret, pos, buf->empty->count, buf->full->count, *size);

   // perform a v operation on empty, makes more empty
   semaphore_v (buf->empty);

   return ret;
}

/* get the current number of elements the thbuf is storing */
gint thbuf_current_size (thbuf_t *buf)
{
   gint ret;
   g_mutex_lock (buf->mutex);
   ret = buf->full->count;
   g_mutex_unlock (buf->mutex);
   return ret;
}

/* get the current number of elements the thbuf is storing without locking */
static gint thbuf_current_size_no_lock (thbuf_t *buf)
{
   return buf->full->count;
}

/* resets a thbuf (frees all data ps) */
void thbuf_clear (thbuf_t *buf)
{
   gint i;
   gint count;

   g_mutex_lock (buf->mutex);

   if (!(count = thbuf_current_size_no_lock (buf))) {
      g_mutex_unlock (buf->mutex);
      LOG ("buffer is already cleared!");
      return;
   }
   
   while (1) {
      void *p = thbuf_consume_no_lock (buf, &count);
      if (p != NULL)
	 g_free (p);
      if (!count)
	 break;
   }

   g_mutex_unlock (buf->mutex);
}

/* allocate a new thbuf */
thbuf_t *thbuf_new (size_t size)
{
   thbuf_t *buf;
   gint i;

   buf = (thbuf_t *)g_malloc (sizeof (thbuf_t));
   buf->size = size;

   // allocate size members of p's and the size array 
   buf->buf = (void **)g_malloc (sizeof (void *) * (size) + 1);

   // initialize all members to 0
   for (i = 0; i < size; i++) {
      buf->buf[i] = NULL;
   }

   // allocate the threading structures 
   buf->mutex = g_mutex_new ();
   buf->empty = semaphore_new (size);
   buf->full = semaphore_new (0);
   buf->produce_pos = 0;
   buf->consume_pos = 0;
  
   return buf;
}

/* free a thbuf */
void thbuf_free (thbuf_t *buf)
{
   g_mutex_free (buf->mutex);
   semaphore_free (buf->empty);
   semaphore_free (buf->full);
   g_free (buf->buf);
   g_free (buf);
}

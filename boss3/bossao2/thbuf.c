#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "thbuf.h"

int thbuf_add (thbuf_s *buf, void *p, size_t size)
{
  if (buf->full)
    return -1;

  buf->buf[buf->tail] = p;
  buf->chunk_size[buf->tail] = size;

  buf->tail++;
  if (buf->tail == buf->size)
    buf->tail = 0;
  if (buf->tail == buf->head)
    buf->full = 1;
  buf->empty = 0;

  return 0;
}

void *thbuf_rem (thbuf_s *buf, size_t *size)
{
  if (buf->empty)
    return NULL;

  void *ret = buf->buf[buf->head];
  *size = buf->chunk_size[buf->head];
  buf->chunk_size[buf->head] = 0;
  //printf ("got %p with a size of %d\n", ret, *size);

  buf->head++;
  if (buf->head == buf->size)
    buf->head = 0;
  if (buf->head == buf->tail)
    buf->empty = 1;
  buf->full = 0;

  return ret;
}

void thbuf_clear (thbuf_s *buf)
{
  int i, size;
  void *this;
  
  this = thbuf_rem (buf, &size);
  while (this != NULL) {
    free (this);
    this = thbuf_rem (buf, &size);
  }

  for (i = 0; i < buf->size; i++) {
    buf->chunk_size[i] = 0;
    buf->buf[i] = NULL;
  }

  buf->full = 1;
  buf->empty = 1;
  buf->head = 0;
  buf->tail = 0;
}

thbuf_s *thbuf_new (size_t size)
{
  thbuf_s *buf;
  int i;

  buf = (thbuf_s *)malloc (sizeof (thbuf_s));

  buf->size = size;
  buf->buf = (void **)malloc (size + 1);
  buf->chunk_size = (size_t *)malloc (sizeof (size_t) * (size + 1));

  for (i = 0; i < size; i++) {
    buf->chunk_size[i] = 0;
    buf->buf[i] = NULL;
  }

  buf->empty = 1;
  buf->full = 0;
  buf->paused = 0;
  buf->head = 0;
  buf->tail = 0;

  buf->mutex = (pthread_mutex_t *)malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (buf->mutex, NULL);
  buf->not_full = (pthread_cond_t *)malloc (sizeof (pthread_cond_t));
  pthread_cond_init (buf->not_full, NULL);
  buf->not_empty = (pthread_cond_t *)malloc (sizeof (pthread_cond_t));
  pthread_cond_init (buf->not_empty, NULL);
  buf->not_paused = (pthread_cond_t *)malloc (sizeof (pthread_cond_t));
  pthread_cond_init (buf->not_paused, NULL);

  return buf;
}

void thbuf_free (thbuf_s *buf)
{
  pthread_mutex_destroy (buf->mutex);
  free (buf->mutex);
  pthread_cond_destroy (buf->not_full);
  free (buf->not_full);
  pthread_cond_destroy (buf->not_empty);
  free (buf->not_empty);
  pthread_cond_destroy (buf->not_paused);
  free (buf->not_paused);
  free (buf->buf);
  free (buf->chunk_size);
  free (buf);
}


typedef struct thbuf_t {
  void **buf;
  size_t *chunk_size;
  size_t head, tail;
  size_t size;
  int full, empty, paused;
  //int readers, writers, waiting;
  pthread_mutex_t *mutex;
  pthread_cond_t *not_full, *not_empty, *not_paused;
  void *p;
} thbuf_s;

int thbuf_add (thbuf_s *buf, void *p, size_t size);
void *thbuf_rem (thbuf_s *buf, size_t *size);
void thbuf_clear (thbuf_s *buf);

//void thbuf_read_lock (thbuf_s *buf, int d);
//void thbuf_read_unlock (thbuf_s *buf);
//void thbuf_write_lock (thbuf_s *buf, int d);
//void thbuf_write_unlock (thbuf_s *buf);

thbuf_s *thbuf_new (size_t size);
void thbuf_free (thbuf_s *buf);


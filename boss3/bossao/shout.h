
shout_t *bossao_shout_init (PyObject *cfgparser);
void shout_encode_chunk (song_s *song, unsigned char *buffer, int size);
void shout_clear (song_s *song);
int shout_encoder_init (song_s *song, PyObject *cfgparser);

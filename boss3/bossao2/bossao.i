%module bossao
%{
#include <Python.h>
#include <ao/ao.h>

#import "bossao.h"
%}
void bossao_new (PyObject *cfgparser);
int bossao_start (song_s *song, PyObject *cfgparser);
int bossao_seek (song_s *song, double secs);
void bossao_shutdown (song_s *song);
void bossao_stop (song_s *song);
int bossao_play (song_s *song, char *filename);
int bossao_pause (song_s *song);
int bossao_unpause (song_s *song);
int bossao_finished (song_s *song);
double bossao_time_total (song_s *song);
double bossao_time_current (song_s *song);
char *bossao_filename (song_s *song);
char *bossao_driver_name (song_s *song);
void bossao_thread_init (void);

%module bossao
%{
#include <Python.h>
#include <ao/ao.h>

#include "bossao.h"
%}
song_s *bossao_new (void);
int bossao_start (song_s *song, PyObject *cfgparser);
int bossao_seek (song_s *song, double secs);
void bossao_shutdown (song_s *song);
int bossao_play (song_s *song, char *filename);
int bossao_pause (song_s *song);
int bossao_unpause (song_s *song);
int bossao_finished (song_s *song);
double bossao_time_total (song_s *song);
double bossao_time_current (song_s *song);
char *bossao_filename (song_s *song);
char *bossao_driver_name (song_s *song);
mixer_s *bossao_new_mixer(void); //Not really needed... yet
int bossao_open_mixer(mixer_s *mixer, char* mixerloc, char* mixertype);
int bossao_close_mixer(mixer_s *mixer);
int bossao_getvol(mixer_s *mixer);
int bossao_setvol(mixer_s *mixer, int vol);
void bossao_del_mixer(mixer_s *mixer); //Ditto on this one

%module bossao
%{
#include <Python.h>
#include <glib.h>

#import "bossao.h"
%}
// only call once!
   void bossao_new(PyObject *cfgparser, gchar *filename);
// a good idea to call stop first
   void bossao_free(void);
// waits on the producer and consumer threads to exit
   void bossao_join (void);
// open the audio devices (done automatically)
   void bossao_open (PyObject *cfgparser);
// close the audio devices (untested)
   void bossao_close (void); 
// seek specified time delta
   gint bossao_seek (gdouble secs);
// stop playing and unload current file
   void bossao_stop (void);
// play given filename
   gint bossao_play (gchar *filename);
// pause the player
   void bossao_pause (void);
// unpause the player
   void bossao_unpause (void);
// check to see if playing is complete
   gint bossao_finished (void);
// check the total time of the playing file
   gdouble bossao_time_total (void);
// check the current time of the playing file
   gdouble bossao_time_current (void);
// get the filename of the current playing file
   gchar *bossao_filename (void);

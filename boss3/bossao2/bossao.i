%module bossao
%{
#include <Python.h>
#include <glib.h>

#import "bossao.h"
%}
   void bossao_new(PyObject *cfgparser, gchar *filename);
   void bossao_free(void);
   
   void bossao_join (void);

   void bossao_open (PyObject *cfgparser);
   void bossao_close (void);
   
   gint bossao_seek (gdouble secs);
   void bossao_stop (void);
   gint bossao_play (gchar *filename);
   void bossao_pause (void);
   void bossao_unpause (void);
   gint bossao_finished (void);
   gdouble bossao_time_total (void);
   gdouble bossao_time_current (void);
   gchar *bossao_filename (void);

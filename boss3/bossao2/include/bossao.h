/*
 * Boss Ogg - A Music Server
 * (c)2003 by Adam Torgerson (may1937@users.sf.net)
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

//#ifndef _BOSSAO_H
//#define _BOSSAO_H

#import <Python.h>

#import "common.h"
#import "thbuf.h"
#import "input_plugin.h"

#define BUF_SIZE 8192
#define BUF_CHUNKS 256

#define RATE 44100

#ifdef __cplusplus
extern "C" {
#endif
   
   gpointer get_symbol (GModule *lib, gchar *name);
   
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
   
   //#endif
#ifdef __cplusplus
}
#endif

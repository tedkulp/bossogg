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

 * some common things across bossao
 
*/

#import <stdlib.h>
#import <stdio.h>
#import <glib.h>
#import <gmodule.h>

#define FN __FUNCTION__
#define LN __LINE__
#define FL __FILE__

/* simple logging stuff */
#define USE_LOGGING
#ifdef USE_LOGGING
#define LOG(msg,args...) printf ("[%s:%d]:%s(): \"", FL, LN, FN); \
  printf (msg, ## args); \
  printf ("\"\n");
#else
#define LOG(msg,args...)
#endif

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

 * test program for bossao2 (woohoo!)
 
*/

#import "bossao.h"
#import "input_plugin.h"
#import "output_plugin.h"
#import "thbuf.h"

int main (int argc, char *argv[])
{
   if (argc != 4) {
      printf ("Usage: %s <filename1.ogg> <filename2.ogg> <secs-to-sleep>\n", argv[0]);
      exit (-1);
   }
   
   bossao_new (NULL, NULL);
   bossao_play (argv[1]);

   sleep (2);

   LOG ("pausing");
   bossao_pause ();
   sleep (1);
   LOG ("unpausing");
   bossao_unpause ();
   bossao_pause ();
   LOG ("paused2");
   bossao_unpause ();

   sleep (2);

   LOG ("stopping");
   bossao_stop ();
   LOG ("stopped");
   bossao_play (argv[2]);
   LOG ("playing...");

   while (!bossao_finished ())
      usleep (10000);
   
   //LOG ("joining..");
   //bossao_join ();

   return 0;
}

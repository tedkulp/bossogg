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

 * libao output plugin
 
*/

#import "output_plugin.h"
#import <alsa/asoundlib.h>

static gchar *plugin_name = "alsa";
static snd_pcm_t *playback_handle;
static gchar *device = "plughw:0,0";

gint output_open (PyObject *cfgparser)
{
   gint err;
   gint exact;
   snd_pcm_hw_params_t *hw_params;
   
   snd_pcm_hw_params_alloca (&hw_params);
   if ((err = snd_pcm_open (&playback_handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
      LOG ("cannot open audio device '%s', error: %s", device, snd_strerror (err));
      return -1;
   }
   /*
   if ((err = snd_pcm_hw_params_alloca (&hw_params)) < 0) {
      LOG ("cannot allocate hw param struct: %s", snd_strerror (err));
      return -2;
      }
   */
   if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
      LOG ("cannot initialize hw params: %s", snd_strerror (err));
      return -3;
   }
   if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params,
					    SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
      LOG ("cannot set access type: %s", snd_strerror (err));
      return -4;
   }
   if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params,
					    SND_PCM_FORMAT_S16_LE)) < 0) {
      LOG ("cannot set sample format: %s", snd_strerror (err));
      return -5;
   }
   snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, 44100, &exact);
   /*
   if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, 44100, &exact)) < 0) {
      LOG ("cannot set sample rate: %s", snd_strerror (err));
      return -6;
      }
   */
   if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 2)) < 0) {
      LOG ("cannot set channel count: %s", snd_strerror (err));
      return -7;
   }
   if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
      LOG ("cannot set hw params: %s", snd_strerror (err));
      return -8;
   }
   //snd_pcm_hw_params_free (hw_params);

   if ((err = snd_pcm_prepare (playback_handle)) < 0) {
      LOG ("cannot prepare audio device for use: %s", snd_strerror (err));
      return -9;
   }
   
   return 0;
}

void output_close (void)
{
   snd_pcm_close (playback_handle);
}

gint output_write_chunk (guchar *buffer, gint size)
{
   gint err;
   if ((err = snd_pcm_writei (playback_handle, buffer, size)) != size) {
      LOG ("write to audio interface of size %d failed: %s", size, snd_strerror (err));
      return -1;
   }
   
   return 0;
}

gchar *output_name (void)
{
   return plugin_name;
}

gchar *output_driver_name (void)
{
   return "ALSA";
   /*
   ao_info *info;

   if (device == NULL) {
      LOG ("Tried to get the driver name of a NULL device");
      return NULL;
   }

   info = ao_driver_info (driver_id);
   return info->short_name;
   */
}

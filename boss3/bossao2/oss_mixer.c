/*
 * Boss Ogg - A Music Server
 * (c)2003 by Ted Kulp (wishy@users.sf.net)
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
#include <Python.h>

//#include <config.h>
#include "oss_mixer.h"

int oss_open_mixer(char* mixerloc) {
	int fd;
	fd = open(mixerloc, O_RDONLY);
	//printf("fd: %d\n", fd);
	return fd;
}

int oss_close_mixer(int fd) {
	int result = 0;
	if (fd > -1) {
		result = 1;
		close(fd);
	}
	return result;
}

int oss_getvol(int fd) {

	int left, right, level;

	//printf("fd: %d\n", fd);
	ioctl(fd,MIXER_READ(SOUND_MIXER_VOLUME),&level);

	left = level & 0xff;
	right = (level & 0xff00) >> 8;

	return left;
}

int oss_setvol(int fd, int vol) {

	int result = 0;
	int level;

	if (vol < 0) vol = 0;
	if (vol > 100) vol = 100;

	level = (vol << 8) + vol;

	if (ioctl(fd, MIXER_WRITE(SOUND_MIXER_VOLUME),&level) >= 0) {
		result = 1;
	}

	return result;
}

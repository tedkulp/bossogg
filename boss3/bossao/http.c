/*
 * Boss Ogg - A Music Server
 * (c)2003 by Adam Torgerson (may1937@users.sf.net)
 * This project's homepage is: http://bossogg.wishy.org
 *
 * This code borrows heavily from XMMS's http.c, Copyright (C) 1998-2000
 * Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define min3(x,y,z) (min(x,y)<(z)?min(x,y):(z))
#define min4(x,y,z,w) (min3(x,y,z)<(w)?min3(x,y,z):(w))

static char *ice_name = NULL;
static int ice_metaint = 0;

static int udp_establish_listener (int *sock);
static int udp_check_for_data (int sock);

static int prebuffering, going, eof = 0;
static int sock, rd_index, wr_index, buffer_length, prebuffer_length;
static long long buffer_read = 0;
static char *buffer;
static pthread_t thread;

static FILE *out_file = NULL;

#define BASE64_LENGTH(len) (4 * (((len) + 2) / 3))

/* Encode the string S of length LENGTH to base64 format and place it
   to STORE.  STORE will be 0-terminated, and must point to a writable
   buffer of at least 1+BASE64_LENGTH(length) bytes.  */
static void base64_encode(const char * s, char * store, int length)
{
    /* Conversion table.  */
    static char tbl[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', '+', '/'
    };
    int i;
    unsigned char *p = (unsigned char *) store;

    /* Transform the 3x8 bits to 4x6 bits, as required by base64.  */
    for (i = 0; i < length; i += 3) {
        *p++ = tbl[s[0] >> 2];
        *p++ = tbl[((s[0] & 3) << 4) + (s[1] >> 4)];
        *p++ = tbl[((s[1] & 0xf) << 2) + (s[2] >> 6)];
        *p++ = tbl[s[2] & 0x3f];
        s += 3;
    }
    /* Pad the result if necessary...  */
    if (i == length + 1)
        *(p - 1) = '=';
    else if (i == length + 2)
        *(p - 1) = *(p - 2) = '=';
    /* ...and zero-terminate it.  */
    *p = '\0';
}

/* Create the authentication header contents for the `Basic' scheme.
   This is done by encoding the string `USER:PASS' in base64 and
   prepending `HEADER: Basic ' to it.  */
static char *basic_authentication_encode(const char * user,
                                          const char * passwd,
                                          const char * header)
{
    char *t1, *t2, *res;
    int len1 = strlen(user) + 1 + strlen(passwd);
    int len2 = BASE64_LENGTH(len1);
    int size;

    size = len1 + strlen (":");
    t1 = malloc (size);
    snprintf(t1, size, "%s:%s", user, passwd);
    t2 = malloc (len2 + 1);
    base64_encode(t1, t2, len1);
    size = strlen (header) + strlen (t2) + strlen (": Basic ") + 3;
    res = malloc (size);
    snprintf(res, size, "%s: Basic %s\r\n", header, t2);
    free(t2);
    free(t1);

    return res;
}

static void parse_url(const char * url, char ** user, char ** pass,
                      char ** host, int * port, char ** filename)
{
    char *h, *p, *pt, *f, *temp, *ptr;

    temp = strdup(url);
    ptr = temp;

    if (!strncasecmp("http://", ptr, 7))
        ptr += 7;
    h = strchr(ptr, '@');
    f = strchr(ptr, '/');
    if (h != NULL && (!f || h < f)) {
        *h = '\0';
        p = strchr(ptr, ':');
        if (p != NULL && p < h) {
            *p = '\0';
            p++;
            *pass = strdup(p);
        } else
            *pass = NULL;
        *user = strdup(ptr);
        h++;
        ptr = h;
    } else {
        *user = NULL;
        *pass = NULL;
        h = ptr;
    }
    pt = strchr(ptr, ':');
    if (pt != NULL && (f == NULL || pt < f)) {
        *pt = '\0';
        *port = atoi(pt + 1);
    } else {
        if (f)
            *f = '\0';
        *port = 80;
    }
    *host = strdup(h);

    if (f)
        *filename = strdup(f + 1);
    else
        *filename = NULL;
    free(temp);
}

void mpg123_http_close(void)
{
    going = 0;

    pthread_join(thread, NULL);
    free(ice_name);
    ice_name = NULL;
}


static int http_used(void)
{
    if (wr_index >= rd_index)
        return wr_index - rd_index;
    return buffer_length - (rd_index - wr_index);
}

static int http_free(void)
{
    if (rd_index > wr_index)
        return (rd_index - wr_index) - 1;
    return (buffer_length - (wr_index - rd_index)) - 1;
}

static void http_wait_for_data(int bytes)
{
#warning "might need something to replace mpg123_info->going"
    while ((prebuffering || http_used() < bytes) && !eof && going)
        usleep(10000);
}

static void show_error_message (char * error)
{
  printf ("http error: %s\n", error);
}

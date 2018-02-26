
//
// ## hyphop ## 
// osc 2 midi
// simple redirect osc midi messages to midi 
// based on oscdump ( liblo )
// 
// how to compile 
// gcc -o osc2midi osc2midi.c -llo
//

/*
 * oscdump - Receive and dump OpenSound Control messages.
 *
 * Copyright (C) 2008 Kentaro Fukuchi <kentaro@fukuchi.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include "config.h"
#include <lo/lo.h>

#include <linux/soundcard.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

int done = 0;
int bundled = 0;
lo_timetag tt_now;
lo_timetag tt_bundle;

#define NAME	"osc2midi"
#define MIDIDEV "MIDIDEV"
#define VERBOSE "VERBOSE"

#ifndef MIDI_DEVICE
#define MIDI_DEVICE "/dev/snd/midiC3D0"
#endif

#ifndef VERSION
#define VERSION "0.29"
#endif

char* device =  MIDI_DEVICE;
int fd;
int verbose = 0;


void usage(void)
{
    printf("%s - simple redirect osc midi messages to midi by ## hyphop ##\n"
    	   "base on oscdump %s Copyright (C) 2008 Kentaro Fukuchi\n\n"
           "Usage: [ENV] %s [-L] <port>\n"
           "or     [ENV] %s [-L] <url>\n\n"
           "Description:\n"
		   "env %s	: set midi device %s=/dev/midi0 %s\n"
		   "env %s	: 1 - display only midi messages, 2 - display all messages \n"
		   "		  by default not display any osc message just redirect\n"
           "-L		: specifies line buffering even if stdout is a pipe or file\n"
           "port		: specifies the listening port number.\n"
           "url		: specifies the server parameters using a liblo URL.\n\n"
           "          e.g. UDP        \"osc.udp://:9000\"\n"
           "               Multicast  \"osc.udp://224.0.1.9:9000\"\n"
           "               TCP        \"osc.tcp://:9000\"\n\n" , 
			
			NAME, VERSION, NAME, NAME, MIDIDEV, MIDIDEV, NAME, VERBOSE );
}

int bundleStartHandler(lo_timetag tt, void *user_data)
{
    if (tt.sec == LO_TT_IMMEDIATE.sec &&
        tt.frac == LO_TT_IMMEDIATE.frac)
    {
        lo_timetag_now(&tt_now);
        tt_bundle.sec = tt_now.sec;
        tt_bundle.frac = tt_now.frac;
    }
    else {
        tt_bundle.sec = tt.sec;
        tt_bundle.frac = tt.frac;
    }
    bundled = 1;
    return 0;
}

int bundleEndHandler(void *user_data)
{
    bundled = 0;
    return 0;
}

void errorHandler(int num, const char *msg, const char *where)
{
    printf("liblo server error %d in path %s: %s\n", num, where, msg);
}

int messageHandler(const char *path, const char *types, lo_arg ** argv,
                   int argc, lo_message msg, void *user_data)
{
    int i;

	if (verbose > 1) {

    if (bundled) {
        printf("%08x.%08x %s %s", tt_bundle.sec, tt_bundle.frac, path, types);
    }
    else {
        lo_timetag tt = lo_message_get_timestamp(msg);
        if (tt.sec == LO_TT_IMMEDIATE.sec &&
            tt.frac == LO_TT_IMMEDIATE.frac)
        {
            lo_timetag_now(&tt_now);
            printf("%08x.%08x %s %s ", tt_now.sec, tt_now.frac, path, types);
        }
        else
            printf("%08x.%08x %s %s ", tt.sec, tt.frac, path, types);
    }

	}

    for (i = 0; i < argc; i++) {

		unsigned int m = argv[i]->i;

			if (verbose > 1) {
	        	lo_arg_pp((lo_type) types[i], argv[i]);
			}

		if ( types[i] == LO_MIDI ) {

			if (verbose == 1) {
	        	lo_arg_pp((lo_type) types[i], argv[i]);
			}

		    unsigned char data[3] = { m >> 24 , m >> 16 & 0xFF,  m >> 8 & 0xFF };

//			printf("MIDI: %08x\n",  m );

//			unsigned char data[3] = {0x90, 60, 127};

		    int r = write(fd, data, sizeof(data));

			if ( !r ) {
				printf("Error: cannot write to %s\n", device);
				exit(1);
			}

		}

    }


	if (verbose ) printf("\n");

    return 0;
}

void ctrlc(int sig)
{
    done = 1;
}


int main(int argc, char **argv)
{
    lo_server server;
    char *port=0, *group=0;
    int i=1;

	if ( getenv(MIDIDEV) ) {
		device = getenv(MIDIDEV);
	}

	if ( getenv(VERBOSE) ) {
		verbose = atoi(getenv(VERBOSE));
	}



    if (argc > i && argv[i][0]=='-') {
#ifdef HAVE_SETVBUF
        if (argv[i][1]=='L') { // line buffering
            setvbuf(stdout, 0, _IOLBF, BUFSIZ);
            i++;
        }
        else
#endif
        if (argv[i][1]=='h') {
            usage();
            exit(0);
        }
        else {
            printf("Unknown option `%s'\n", argv[i]);
            exit(1);
        }
    }

    if (argc > i) {
        port = argv[i];
        i++;
    } else {
        usage();
        exit(1);
    }

    if (argc > i) {
        group = argv[i];
    }

   fd = open(device, O_WRONLY, 0);

   if (fd < 0) {
      printf("Error: cannot open %s\n", device);
      exit(1);
   }

	printf( "osc 2 midi %s \n", device );

    if (group) {
        server = lo_server_new_multicast(group, port, errorHandler);
    } else if (isdigit(port[0])) {
        server = lo_server_new(port, errorHandler);
    } else {
        server = lo_server_new_from_url(port, errorHandler);
    }

    if (server == NULL) {
        fprintf(stderr, "Could not start a server with port %s", port);
        if (group)
            fprintf(stderr, ", multicast group %s\n", group);
        else
            fprintf(stderr, "\n");
        exit(1);
    }

    lo_server_add_method(server, NULL, NULL, messageHandler, NULL);
    lo_server_add_bundle_handlers(server, bundleStartHandler, bundleEndHandler,
                                  NULL);

    signal(SIGINT, ctrlc);

    while (!done) {
        lo_server_recv_noblock(server, 1);
    }

	close(fd);
    return 0;
}

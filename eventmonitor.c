// Description:
// polls /dev/input/event* for any update.
//
// Modified by Jeff Strunk
// Originally EventMonitor.c from keywatcher
// Copyright (C) 2002-2005 Frank Becker
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation;	either version 2 of the License,	or (at your option) any	later
// version.
//
// This program is distributed in the hope that it will be useful,	but	WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details
//
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/input.h>

#include "eventmonitor.h"

void initializeIE(void)  {
	int j=0;
	int tmpfd;
	if (eventData.events[0] == NULL) {
		int i;
		int result;

		for (i=0; i<MAX_CHANNELS; i++) {
			char devName[128];
			snprintf(devName, 127, "/dev/input/event%d",i);
				result = access(devName, R_OK);
				if (result == 0) {
					eventData.events[j] = devName;
					j++;
				}
			}
			eventData.events[j] = NULL;
		}

		if (eventData.events[0] == NULL) {
			fprintf(stderr,"sleepd: there are no event files to watch.\n");
			exit(1);
		}

		j=0;
		while (eventData.events[j] != NULL) {
			tmpfd = open(eventData.events[j], O_RDONLY);
			if (tmpfd != -1) {
				eventData.channels[j] = tmpfd;
				j++;
			}
		}
		eventData.channels[j] = -1;
}

void cleanupIE(void)  {
	int i;
	for (i=0; eventData.channels[i] != -1; i++) {
		close (eventData.channels[i]);
	}
}

void *waitForInputEvent(void *threadid)  {
	int tid = (int)threadid;
	struct timeval tv;
	int retval;
	int *activity = eventData.activity;
	fd_set eventWatch;

	int i;
	for (i=0; i < eventData.timeout; i++) {
		if (*activity == 0) {
			FD_ZERO(&eventWatch);
			FD_SET( eventData.channels[tid], &eventWatch);

			tv.tv_sec = 1;
			tv.tv_usec = 0;
			retval = select(eventData.channels[tid] + 1, &eventWatch, NULL, NULL, &tv);

			if (retval > 0 ) {
				*activity = 1;
				break;
			}
		}
		else {
			break;
		}
	}
	pthread_exit(NULL);
}

void *eventMonitor() {
	int rc;
	initializeIE();
//	int *activity = eventData.activity;
	int event=0;
	while (eventData.channels[event] != -1) {
		rc = pthread_create(&eventData.tid[event], NULL, waitForInputEvent, (void *) event);
		event++;
	}
	sleep(eventData.timeout);

	event=0;
	while (eventData.channels[event] != -1) {
		rc = pthread_join(eventData.tid[event], NULL);
		event++;
	}
	cleanupIE();
	pthread_exit(NULL);
}

/*
int main()
{
	int activity;
	//example of intended use
	pthread_t mainthread;
	//every thread needs to see and be able to modify activity
	eventData.activity = &activity;
	eventData.timeout = 10;
	//start loops here
	activity = 0;
	//start the eventMonitor
	pthread_create(&mainthread, NULL, eventMonitor, NULL);
	// Do other stuff here.
	// When other stuff is complete, don't run sleep.
	// eventMonitor sleeps for timeout, and pthread_join blocks until it exits.
	pthread_join(mainthread,NULL);
	printf("activity=%d\n", activity);
	//end loops here

	return 0;
}
*/

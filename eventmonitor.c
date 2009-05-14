// Description:
//
// Copyright (C) 2002-2005 Frank Becker
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation;  either version 2 of the License,  or (at your option) any  later
// version.
//
// This program is distributed in the hope that it will be useful,  but  WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/input.h>
#include <defines.h>

#define MAX_CHANNELS    64
static int eventChannels[MAX_CHANNELS];
fd_set eventWatch;
void initializeIE(void) 
{
    int i;
    for( i=0; i<MAX_CHANNELS; i++)
    {
	char devName[128];
	snprintf( devName, 127, "/dev/input/event%d",i);
	eventChannels[i] = open(devName, O_RDONLY);
	if( eventChannels[i] == -1)
	{
	    break;
	}
	printf("Adding %s %d\n", devName, eventChannels[i]);
    }
}

void cleanupIE(void) 
{
    int i;
    for( i=0; eventChannels[i] != -1; i++)
    {
	close( eventChannels[i]);
    }
}

int waitForInputEvent(void) 
{
    struct timeval tv;
    int retval;
    int i;
    int maxfd = 0;
    /* Wait up to five seconds. */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    FD_ZERO(&eventWatch);
    maxfd = 0;
    for( i=0; eventChannels[i] != -1; i++)
    {
	FD_SET( eventChannels[i], &eventWatch);
	if( eventChannels[i] > maxfd) maxfd = eventChannels[i];
    }
    maxfd++;

    retval = select(maxfd, &eventWatch, NULL, NULL, &tv);

    if( retval)
    {
	for( i=0; eventChannels[i] != -1; i++)
	{
	    if( FD_ISSET( eventChannels[i], &eventWatch))
	    {
		struct input_event event;
		int s;
		s = read( eventChannels[i], &event, sizeof(event));
		printf("Event from channel %d\n", i);
		printf( "Read %d, type=%d, code=%d, val=%d\n",
			s, event.type, event.code, event.value);
		return event.code;
	    }
	}
    }
#if 0
    else
	printf("No events...\n");
#endif

    return -1;
}

int main( /*int argc, char *argv[]*/)
{
    printf("keyMonitor - Copyright (C) 2005 by Frank Becker\n");
    printf("keyMonitor %s built %s %s\n", VERSION ,__DATE__, __TIME__);

    initializeIE();
    while(1)
    {
	waitForInputEvent();
    }
    cleanupIE();
    
    return 0;
}

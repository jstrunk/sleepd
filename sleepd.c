/*
 * forced apm sleep daemon
 *
 * Copyright 2000, 2001 Joey Hess <joeyh@kitenet.net> under the terms of the
 * GNU GPL.
 */

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/apm_bios.h>
#include <apm.h>
#include <signal.h>
#include "sleepd.h"

int irqs[MAX_IRQS]; /* irqs to examine have a value of 1 */
int max_unused=10 * 60; /* in seconds */
int check_ac=0;
int ac_max_unused;
char *sleep_command="apm -s";
int autoprobe=1;
int daemonize=1;
int have_irqs=0;
int sleep_time = DEFAULT_SLEEP_TIME;
int no_sleep=0;
int min_batt=0;

void usage () {
	fprintf(stderr, "Usage: sleepd [-s command] [-u n] [-U n] [-i n [-i n ..]] [-a] [-n] [-c n] [-b n]\n");
}

void parse_command_line (int argc, char **argv) {
	extern char *optarg;
	struct option long_options[] = {
		{"nodaemon", 0, NULL, 'n'},
		{"unused", 1, NULL, 'u'},
		{"ac-unused", 1, NULL, 'U'},
		{"irq", 1, NULL, 'i'},
		{"help", 0, NULL, 'h'},
		{"sleep-command", 1, NULL, 's'},
		{"auto", 0, NULL, 'a'},
		{"check-period", 1, NULL, 'c'},
		{"battery", 1, NULL, 'b'},
		{0, 0, 0, 0}
	};
	int force_autoprobe=0;
	int c=0;
	int i;

	while (c != -1) {
		c=getopt_long(argc,argv, "nu:U:wi:hac:b:", long_options, NULL);
		switch (c) {
			case 's':
				sleep_command=strdup(optarg);
				break;
			case 'n':
				daemonize=0;
				break;
			case 'u':
				max_unused=atoi(optarg);
				break;
			case 'U':
				if (apm_exists() != 0) {
					fprintf(stderr, "sleepd: apm support not present in kernel\n");
					exit(1);
				}
				check_ac=1;
				ac_max_unused=atoi(optarg);
				break;
			case 'i':
				i = atoi(optarg);
				if ((i < 0) || (i >= MAX_IRQS)) {
					fprintf(stderr, "sleepd: bad irq number %d\n", i);
					exit(1);
				}
				irqs[atoi(optarg)]=1;
				autoprobe=0;
				have_irqs=1;
				break;
			case 'h':
				usage();
				exit(0);
				break;
			case 'a':
				force_autoprobe=1;
				break;
			case 'c':
				sleep_time=atoi(optarg);
				if (sleep_time <= 0) {
					fprintf(stderr, "sleepd: bad sleep time %d\n", sleep_time);
					exit(1);
				}
				break;
			case 'b':
				min_batt=atoi(optarg);
				if (min_batt <= 0) {
					fprintf(stderr, "sleepd: bad minimumn battery percentage %d\n", min_batt);
					exit(1);
				}
				break;
		}
	}
	if (optind < argc) {
		usage();
		exit(1);
	}
	if (force_autoprobe)
		autoprobe=1;
}

/* Keep checking the interrupts. As long as there is activity, do nothing. */
void main_loop (void) {
	long irq_count[MAX_IRQS]; /* holds previous counters of the irq's */
	int activity, i, sleep_now=0, total_unused=0, do_this_one=0, probed=0;
	long v;
	time_t nowtime, oldtime=0;
	FILE *f;
	char line[64];
	apm_info ai;
	
	while (1) {
		activity=0;
		f=fopen(INTERRUPTS, "r");
		if (! f) {
			perror(INTERRUPTS);
			exit(1);
		}
		while(fgets(line,sizeof(line),f)) {
			if (autoprobe) {
				/* Lowercase line. */
				for(i=0;line[i];i++)
					line[i]=tolower(line[i]);
				/* See if it is a keyboard or mouse. */
				if ((strstr(line, "mouse") != NULL ||
				    strstr(line, "keyboard") != NULL)) {
					do_this_one=1;
					probed=1;
				}
				else {
					do_this_one=0;
				}
			}
			if (sscanf(line,"%d: %ld",&i, &v) == 2 && 
			    (do_this_one || irqs[i]) && irq_count[i] != v) {
				activity=1;
				irq_count[i] = v;
			}
		}
		fclose(f);
		
		if (autoprobe && ! probed) {
			syslog(LOG_WARNING, "no keyboard or mouse irqs autoprobed");
		}
		
		if (min_batt || check_ac)
			apm_read(&ai);

		if (min_batt && ai.ac_line_status != 1 && 
		    ai.battery_percentage < min_batt) {
			syslog(LOG_NOTICE, "battery is low; forcing sleep");
			if (system(sleep_command) != 0)
				syslog(LOG_ERR, "%s failed", sleep_command);
			/* This counts as activity; to prevent double sleeps. */
			activity=1;
			oldtime=0;
		}
		
		if (activity) {
			total_unused = 0;
		}
		else {
			total_unused += sleep_time;
			if (check_ac && ai.ac_line_status == 1) {
				/* On wall power and it matters. */
				if (ac_max_unused > 0) {
					sleep_now = total_unused >= ac_max_unused;
				}
			}
			else {
				sleep_now = total_unused >= max_unused;
			}

			if (sleep_now && ! no_sleep) {
				syslog(LOG_NOTICE, "system is inactive; forcing sleep");
				if (system(sleep_command) != 0)
					syslog(LOG_ERR, "%s failed", sleep_command);
				total_unused=0;
				oldtime=0;
				sleep_now=0;
			}
		}
		
		sleep(sleep_time);
		
		/*
		 * Keep track of how long it's been since we were last
		 * here. If it was much longer than sleep_time, the system
		 * was probably suspended, or this program was, (or the 
		 * kernel is thrashing :-), so clear idle counter.
		 */
		nowtime=time(NULL);
		/* The 1 is a necessary fudge factor. */
		if (oldtime && nowtime - sleep_time > oldtime + 1) {
			no_sleep=0; /* reset, since they must have put it to sleep */
			syslog(LOG_NOTICE,
					"%i sec sleep; resetting timer",
					(int)(nowtime - oldtime));
			total_unused=0;
		}
		oldtime=nowtime;
	}
}

void loadcontrol (int signum) {
	int f;
	char buf[8];
	
	if (((f=open(CONTROL_FILE, O_RDONLY)) == -1) ||
            (flock(f, LOCK_SH) == -1) ||
	    (read(f, buf, 7) == -1))
		return;
        no_sleep=atoi(buf);
	close(f);

	signal(SIGHUP, loadcontrol);
}

void cleanup (int signum) {
	if (daemonize)
		unlink(PID_FILE);
	exit(0);
}

int main (int argc, char **argv) {
	FILE *f;

	parse_command_line(argc, argv);
	
	/* Log to the console if not daemonizing. */
	openlog("sleepd", LOG_PID | (daemonize ? 0 : LOG_PERROR), LOG_DAEMON);
	
	/* Set up a signal handler for SIGTERM to clean up things. */
	signal(SIGTERM, cleanup);
	/* And a handler for SIGHUP, to reaload control file. */
	signal(SIGHUP, loadcontrol);
	loadcontrol(0);
	
	if (! have_irqs && ! autoprobe) {
		fprintf(stderr, "No irqs specified.\n");
		exit(1);
	}

	if (daemonize) {
		if (daemon(0,0) == -1) {
			perror("daemon");
			exit(1);
		}
		if ((f=fopen(PID_FILE, "w")) == NULL) {
			syslog(LOG_ERR, "unable to write %s", PID_FILE);
			exit(1);
		}
		else {
			fprintf(f, "%i\n", getpid());
			fclose(f);
		}
	}
	
	main_loop();
	
	return(0); // never reached
}

/*
 * File: test.c
 *
 * Test program for locking routines
 *
 * Bob Eager   August 2003
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <ctype.h>

#include "lock.h"
#include "log.h"

#define	LOGNAME		"z.log"

static	char	*dirname = "f:\\PostOffice\\rde";

int main(int argc, char *argv[])
{	int rc;
	int c;

	rc = open_logfile("TMP", LOGNAME);

	for(;;) {
		fprintf(stdout, "Enter L to lock, U to unlock, Q to quit:");
		fflush(stdout);
		c = getch();
		fputc('\n', stdout);

		switch(toupper(c)) {
			case 'L':
				rc = lock(dirname);
				fprintf(stdout, "Lock called, rc = %d\n", rc);
				break;

			case 'U':
				unlock();
				fprintf(stdout, "Unlocked...\n");
				break;

			case 'Q':
				close_logfile();
				exit(EXIT_SUCCESS);

			case '\r':
			case '\n':
				break;

			default:
				fprintf(stdout, "?\n");
				break;
		}
	}
}

/*
 * End of file: test.c
 *
 */


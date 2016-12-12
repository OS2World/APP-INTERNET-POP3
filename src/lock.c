/*
 * File: lock.c
 *
 * Maildrop locking routines
 *
 * Bob Eager   December 2003
 *
 */

#pragma	strings(readonly)

#include <stdio.h>
#include <string.h>

#define	INCL_DOSERRORS
#define	INCL_DOSSEMAPHORES
#include <os2.h>

#include "lock.h"
#ifdef	DEBUG
#include "log.h"
#endif

#define	MAXSEM		255		/* Max length of semaphore name */

/* Local storage */

static	UCHAR	semname[MAXSEM+1];
static	HMTX	hmtx;


/*
 * Lock a maildrop. The argument specifies the directory
 * name; this is used to derive a suitable semaphore name.
 *
 * Returns:
 *	TRUE		lock successfully acquired
 *	FALSE		lock not acquired
 *
 */

BOOL lock(PUCHAR dirname)
{	PUCHAR p;
	APIRET rc;

	sprintf(semname, "\\SEM32\\%s", dirname);
	p = strchr(semname, ':');
	if(p != (PUCHAR) NULL) *p = 'X';	/* Make legal */
	(void) strupr(semname);			/* Avoid ambiguity */

	rc = DosCreateMutexSem(semname, &hmtx, 0L, FALSE);
#ifdef	DEBUG
	trace("DosCreateMutexSem => %d", rc);
#endif

	if(rc == ERROR_DUPLICATE_NAME) {
		rc = DosOpenMutexSem(semname, &hmtx);
#ifdef	DEBUG
		trace("DosOpenMutexSem => %d", rc);
#endif
		if(rc != 0) {
			(void) DosCloseMutexSem(hmtx);
		}
	}

	rc = DosRequestMutexSem(hmtx, SEM_IMMEDIATE_RETURN);
#ifdef	DEBUG
	trace("DosRequestMutexSem => %d", rc);
#endif
	if(rc != 0) {
		(void) DosCloseMutexSem(hmtx);
	}

	return(rc == 0 ? TRUE : FALSE);
}


/*
 * Unlock the locked maildrop.
 *
 */

VOID unlock(VOID)
{	APIRET rc;

	rc = DosReleaseMutexSem(hmtx);
#ifdef	DEBUG
	trace("DosReleaseMutexSem => %d", rc);
#endif
	rc = DosCloseMutexSem(hmtx);
#ifdef	DEBUG
	trace("DosCloseMutexSem => %d", rc);
#endif
}

/*
 * End of file: lock.c
 *
 */

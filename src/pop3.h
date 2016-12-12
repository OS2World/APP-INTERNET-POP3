/*
 * File: pop3.h
 *
 * POP3 client for Tavi network
 *
 * Header file
 *
 * Bob Eager   December 2003
 *
 */

#include "log.h"

#define	VERSION			4	/* Major version number */
#define	EDIT			1	/* Edit number within major version */

#define	FALSE			0
#define	TRUE			1

/* External references */

extern	VOID	error(PUCHAR mes, ...);
extern	BOOL	client(INT, PUCHAR, PUCHAR, PUCHAR, PUCHAR, PUCHAR,
			PUCHAR, BOOL, BOOL, BOOL, INT, INT);
extern	PVOID	xmalloc(size_t);

/*
 * End of file: pop3.h
 *
 */

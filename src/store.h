/*
 * File: store.h
 *
 * General mail storage routines; header file.
 *
 * Bob Eager   December 2003
 *
 */

/* Miscellaneous constants */

#define	MAXMAILID		9	/* Maximum length of a mail ID */

#define	FALSE			0
#define	TRUE			1

#ifndef	NOLOG
#define	SECURITY_LOG		"I:\\MPTN\\ETC\\SECURITY\\"
#endif

/* Error codes */

#define	STORINIT_OK		0	/* Initialisation successful */
#define	STORINIT_BADDIR		2	/* Cannot access directory */

/* External references */

extern	BOOL	mail_close(VOID);
extern	INT	mail_init(PUCHAR, PUCHAR, INT);
extern	BOOL	mail_open(PUCHAR *);
extern	VOID	mail_reset(VOID);
extern	BOOL	mail_store(PUCHAR);

/*
 * End of file: store.h
 *
 */

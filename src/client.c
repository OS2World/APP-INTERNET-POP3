/*
 * File: client.c
 *
 * POP3 client for Tavi network
 *
 * Protocol handler for client
 *
 * Bob Eager   December 2003
 *
 */

#pragma	strings(readonly)

#define	INCL_DOSPROCESS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "pop3.h"
#include "store.h"
#include "lock.h"
#include "netio.h"

#define	RBUFSIZE	200		/* Size of read buffer */
#define	WBUFSIZE	200		/* Size of write buffer */
#define	RTIMEOUT	30		/* Read timeout (secs) */
#define	WTIMEOUT	30		/* Write timeout (secs) */

#define	MAXLINE		1002		/* Maximum length of line */
#define	MAXMES		100		/* Maximum message length */

#define	MAXLOCKTRIES	5		/* Maximum number of tries at maildrop lock */
#define	LOCKINTERVAL	10		/* Lock retry interval (seconds) */

/* Type definitions */

typedef	enum	{ ST_USER, ST_PASS, ST_STAT, ST_IDLE, ST_RETR, ST_DATA }
	STATE;

/* Forward references */

static	BOOL	get_reply(INT, PUCHAR);
static	VOID	net_read_error(INT);
static	VOID	net_read_timeout(INT);
static	VOID	retrieve_mail(INT, PUCHAR, PUCHAR, PUCHAR, PUCHAR,
				PUCHAR, INT, INT, BOOL, BOOL);
static	BOOL	store_message(INT, PUCHAR, PUCHAR, PUCHAR, PUCHAR, PUCHAR, INT);

/* Local storage */

static	INT	msgcount;
static	PUCHAR	msg_id;			/* Message ID as a string */
static	STATE	state;
static	UCHAR	rbuf[RBUFSIZE+1];
static	UCHAR	wbuf[WBUFSIZE+1];


/*
 * Do the conversation between the client and the server.
 *
 * Returns:
 *	TRUE		client ran and terminated
 *	FALSE		client failed
 *
 */

BOOL client(INT sockno, PUCHAR clientname, PUCHAR servername, PUCHAR serverip,
		PUCHAR dirname, PUCHAR username, PUCHAR password,
		BOOL dirspec, BOOL leave, BOOL verbose, INT begin, INT max)
{	BOOL rc;
	INT i;
	INT nmsgs;
	INT msgspace;
	PUCHAR p, q, r;

	state = ST_USER;

	rc = mail_init(username, dirname, dirspec);
	switch(rc) {
		case STORINIT_OK:
			break;

		case STORINIT_BADDIR:
			error("cannot access mail storage directory");
			return(FALSE);

		default:
			error("mail storage initialisation failed");
			return(FALSE);
	}

	if(netio_init() == FALSE) {
		error("network initialisation failure");
		return(FALSE);
	}

	rc = get_reply(sockno, rbuf);
	if(rc == FALSE) return(FALSE);

	msgcount = 0;

	/* Handle the reply to the connect */

	if(rbuf[0] != '+') {		/* Some kind of failure */
		error("connect failed: %s", rbuf);
		dolog(LOG_ERR, rbuf);
		return(FALSE);
	}
	dolog(LOG_INFO, rbuf);

	/* Send USER to open conversation */

	sprintf(wbuf, "USER %s\n", username);
#ifdef	DEBUG
	trace(wbuf);
#endif
	sock_puts(wbuf, sockno, WTIMEOUT);
	rc = get_reply(sockno, rbuf);
	if(rc == FALSE) return(FALSE);

	/* Handle the reply to USER */

	if(rbuf[0] != '+') {		/* Some kind of failure */
		error("USER failed: %s", rbuf);
		dolog(LOG_ERR, rbuf);
		return(FALSE);
	}

	/* Send PASS to complete authorisation */

	state = ST_PASS;
	sprintf(wbuf, "PASS %s\n", password);
#ifdef	DEBUG
	trace(wbuf);
#endif
	sock_puts(wbuf, sockno, WTIMEOUT);
	rc = get_reply(sockno, rbuf);
	if(rc == FALSE) return(FALSE);

	/* Handle the reply to PASS */

	if(rbuf[0] != '+') {		/* Some kind of failure */
		error("PASS failed: %s", rbuf);
		dolog(LOG_ERR, rbuf);
		return(FALSE);
	}

	/* Send STAT to get statistics */

	state = ST_STAT;
	strcpy(wbuf, "STAT\n");
#ifdef	DEBUG
	trace(wbuf);
#endif
	sock_puts(wbuf, sockno, WTIMEOUT);
	rc = get_reply(sockno, rbuf);
	if(rc == FALSE) return(FALSE);

	/* Handle the reply to STAT */

	if(rbuf[0] != '+') {		/* Some kind of failure */
		error("STAT failed: %s", rbuf);
		dolog(LOG_ERR, rbuf);
		return(FALSE);
	}
	p = strtok(rbuf, " \t");
	q = strtok(NULL, " \t");
	r = strtok(NULL, " \t");
	if(strcmp(p, "+OK") != 0) {
		error("STAT returned illegal information: %s", rbuf);
		dolog(LOG_ERR, rbuf);
		return(FALSE);
	}
	nmsgs = atoi(q);
	msgspace = atoi(r);
	sprintf(rbuf, "maildrop for %s has %d message%s occupying %d octets",
		username, nmsgs, nmsgs == 1 ? "": "s", msgspace);
	dolog(LOG_INFO, rbuf);

	state = ST_IDLE;

	/* Lock the local maildrop directory */

	for(i = 1; i <= MAXLOCKTRIES; i++) {
#ifdef	DEBUG
		trace("trying for maildrop lock...");
#endif
		rc = lock(dirname);
		if(rc == TRUE) break;
		if(i == MAXLOCKTRIES) {
			error("failed to lock maildrop");
			dolog(LOG_ERR, "failed to lock maildrop");
			return(FALSE);
		}
#ifdef	DEBUG
		trace("waiting for lock...");
#endif
		(VOID) DosSleep(LOCKINTERVAL*1000);
	}

#ifdef	DEBUG
	trace("locked the maildrop");
#endif
	if(nmsgs > max) nmsgs = max;
	retrieve_mail(sockno, clientname, servername, serverip, username,
		dirname, begin, nmsgs, leave, verbose);

	/* Unlock the maildrop */

	unlock();

	/* Send QUIT to close the conversation */

#ifdef	DEBUG
	trace("QUIT");
#endif
	sock_puts("QUIT\n", sockno, WTIMEOUT);
	rc = get_reply(sockno, rbuf);
	if(rc == FALSE) return(FALSE);

	/* Handle the reply to QUIT */

	if(rbuf[0] != '+') {		/* Some kind of failure */
		error("QUIT failed: %s", rbuf);
		dolog(LOG_ERR, rbuf);
		return(FALSE);
	}

	sprintf(rbuf, "signed off from %s [%d message%s received]",
			servername, msgcount, msgcount == 1 ? "" : "s");
	dolog(LOG_INFO, rbuf);

	return(TRUE);
}


/*
 * Retrieve waiting mail.
 *
 * Returns:
 *	None
 *
 */

static VOID retrieve_mail(INT sockno, PUCHAR clientname, PUCHAR servername,
				PUCHAR serverip, PUCHAR username,
				PUCHAR dirname, INT begin, INT nmsgs,
				BOOL leave, BOOL verbose)
{	INT i;
	BOOL rc;

#ifdef	DEBUG
	trace("starting at message %d of %d messages", begin, nmsgs);
#endif

	for(i = begin; i <= nmsgs; i++) {

		/* Send RETR to initiate retrieval of specified message */

		sprintf(wbuf, "RETR %d\n", i);
#ifdef	DEBUG
		trace(wbuf);
#endif
		state = ST_RETR;
		sock_puts(wbuf, sockno, WTIMEOUT);

		if(verbose == TRUE) {
			fprintf(stdout, "Retrieving message %d of %d\r",
					i, nmsgs);
			fflush(stdout);
		}

		rc = get_reply(sockno, rbuf);
		if(rc == FALSE) continue;

		/* Handle the reply to RETR */

		if(rbuf[0] != '+') {		/* Some kind of failure */
			error("RETR failed: %s", rbuf);
			dolog(LOG_ERR, rbuf);
			continue;
		}

		state = ST_DATA;
		if(store_message(sockno, clientname, servername, serverip,
					dirname, username, i) == FALSE)
			break;

		msgcount++;

		if(leave == FALSE) {		/* Deletion required */
			sprintf(wbuf, "DELE %d\n", i);
#ifdef	DEBUG
			trace(wbuf);
#endif
			sock_puts(wbuf, sockno, WTIMEOUT);
			rc = get_reply(sockno, rbuf);
			if(rc == FALSE) continue;

			/* Handle the reply to DELE */

			if(rbuf[0] != '+') {	/* Some kind of failure */
				error("DELE failed: %s", rbuf);
				dolog(LOG_ERR, rbuf);
			}
		}
	}
	if(verbose == TRUE) {
		fprintf(stdout, "%50s\r", "");
		if(msgcount == 0) {
			fputs("No messages retrieved\n", stdout);
		} else {
			fprintf(
				stdout,
				"%d message%s retrieved\n",
				msgcount,
				msgcount == 1 ? "" : "s");
		}
	}
}


/*
 * Store the current message.
 *
 * Returns:
 *	TRUE	message stored successfully.
 *	FALSE	failed to store message.
 *
 */

static BOOL store_message(INT sockno, PUCHAR clientname, PUCHAR servername,
				PUCHAR serverip, PUCHAR dirname,
				PUCHAR username, INT msgno)
{	BOOL rc;
	INT len;
	INT index;
	time_t tod, utc, utcdiff;
	struct tm gtm;
	UCHAR timeinfo[40];
	UCHAR offset[20];
	UCHAR buf[MAXLINE+1];
	UCHAR buf2[MAXLINE+1];
	UCHAR mes[MAXMES+1];

	rc = mail_open(&msg_id);
	if(rc == FALSE) {
		sprintf(mes, "failed to open file for message %d,", msgno);
		error(mes);
		dolog(LOG_ERR, mes);
		return(FALSE);
	}

	/* Set up timestamp. To be RFC2821 compliant, the timezone
	   must be displayed as an offset. To get that offset, we
	   get the UTC time into a 'tm' structure using gmtime, then
	   convert it back to a 'time_t' so that we can use difftime
	   to get the actual difference. */

	(VOID) time(&tod);
	gtm = *(gmtime(&tod));		/* Get UTC as a structure */
	utc = mktime(&gtm);		/* Convert back to time_t */
	utcdiff = difftime(tod, utc)/60;/* Offset in minutes */
	sprintf(
		offset,
		" %c%02d%02d",
		utcdiff >= 0 ? '+': '-', abs(utcdiff)/60, abs(utcdiff)%60);

	(VOID) strftime(
		timeinfo,
		sizeof(timeinfo),
		"%a, %Od %b %Y %X",
		localtime(&tod));
	strcat(timeinfo, offset);

	sprintf(
		buf,
		"Received: from %s (%s) by %s\n",
		servername,
		serverip,
		clientname);
	sprintf(buf2, "          with POP3 id %s; %s\n", msg_id, timeinfo);
#ifdef	DEBUG
	trace("%s", buf);
	trace("%s", buf2);
#endif
	if(mail_store(buf) == FALSE ||
	   mail_store(buf2) == FALSE) {
		error("failed to store message");
		dolog(LOG_ERR, "failed to store message");
		mail_reset();
		return(FALSE);
	}

	for (;;) {
		index = 0;

		len = sock_gets(buf, sizeof(buf), sockno, RTIMEOUT);
		if(len == SOCKIO_ERR || len == 0) {
			net_read_error(sockno);
			return(FALSE);
		}
		if(len == SOCKIO_TIMEOUT) {
			net_read_timeout(sockno);
			return(FALSE);
		}
		if(len == SOCKIO_TOOLONG) {
			error("line too long");
			dolog(LOG_WARNING, "line too long\n");
			continue;
		}

		if (buf[0] == '.') {
			index = 1;			/* Un-stuff dots */
			if(buf[index] == '\n') break;	/* End of data */
		}
		if(mail_store(&buf[index]) == FALSE) {
			error("failed to store message");
			dolog(LOG_ERR, "failed to store message");
			mail_reset();
			return(FALSE);
		}
#ifdef	DEBUG
		trace("data(%d): %s", strlen(buf), buf);
#endif
	}

	rc = mail_close();
	if(rc == FALSE) {
		sprintf(mes, "failed to close file for message %d,", msgno);
		error(mes);
		dolog(LOG_ERR, mes);
		mail_reset();
		return(FALSE);
	}

	sprintf(mes, "stored message %d, ID %s", msgno, msg_id);
	dolog(LOG_INFO, mes);

	return(TRUE);
}


/*
 * Read a reply from the server.
 *
 * Returns:
 *	TRUE if reply read OK.
 *	FALSE if network read error.
 *
 */

static BOOL get_reply(INT sockno, PUCHAR buf)
{	INT rc;

	rc = sock_gets(buf, RBUFSIZE, sockno, RTIMEOUT);
	if(rc < 0) {
		if(rc == SOCKIO_ERR) {
			error("network read error");
			return(FALSE);
		}
		if(rc == SOCKIO_TIMEOUT) {
			error("network read timeout");
			return(FALSE);
		}
		if(rc == SOCKIO_TOOLONG) {
			error("network input line too long");
			return(FALSE);
		}
	}
#ifdef	DEBUG
	trace("%s", buf);
#endif
	return(TRUE);
}


/*
 * Handle a network read error.
 *
 */

static VOID net_read_error(INT sockno)
{	error("network read error");
	dolog(LOG_ERR, "network read error\n");
	mail_reset();
}


/*
 * Handle a network read timeout.
 *
 */

static VOID net_read_timeout(INT sockno)
{	error("network read timeout");
	dolog(LOG_ERR, "network read timeout\n");
	mail_reset();
}

/*
 * End of file: client.c
 *
 */

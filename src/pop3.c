/*
 * File: pop3.c
 *
 * POP3 client for Tavi network
 *
 * Main program
 *
 * Bob Eager   December 2003
 *
 */

/*
 * History:
 *
 *	1.0	Initial version.
 *	1.1	Better handling of long usernames/passwords.
 *		Correction to generation of mail directory based on
 *		username on FAT volumes.
 *	1.2	Added -n flag to limit number of messages retrieved.
 *	1.3	Added Received: line to incoming mail messages.
 *	1.4	Fixed problem with non fully-qualified receive directory
 *		names.
 *	1.5	Fixed problem with reversed source and destination names
 *		in generated Received: header line.
 *	1.6	Added -v option to display progress of mail collection.
 *	1.7	Use new thread-safe logging module.
 *		Use OS/2 type definitions.
 *		New, simplified network interface module.
 *	1.8	Added option security logging.
 *	1.9	Added final message count if verbose option turned on.
 *		Modified to use NETLIB DLL.
 *	2.0	Added BLDLEVEL string.
 *		Diagnostics for occasional logfile open failures.
 *		Additional error checking in logging module.
 *		Grouped initialisation code together.
 *		Added -b flag to allow retrieval to start at a particular
 *		message.
 *	3.0	Recompiled using 32-bit TCP/IP toolkit, in 16-bit mode.
 *	4.0	Added support for logging to 'syslog' instead of to file,
 *		selectable by '-z' option. This has the advantage of
 *		avoiding clashing logfile usage.
 *		Changes to conform more closely with RFC 2821.
 *		Add client IP to Received: line.
 *		Changed timezone to offset in Received: line, to comply
 *		with RFC2821.
 *		Make timestamps conform to RFC2821/RFC2822 in terms of
 *		leading zeros and four digit year values.
 *	4.1	Added -q flag for quiet operation.
 *
 */

#pragma	strings(readonly)

#pragma	alloc_text(a_init_seg, main)
#pragma	alloc_text(a_init_seg, error)
#pragma	alloc_text(a_init_seg, fix_domain)
#pragma	alloc_text(a_init_seg, log_connection)
#pragma	alloc_text(a_init_seg, putusage)

#include <os2.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <types.h>
#include <limits.h>

#define	OS2
#include <sys\socket.h>
#include <netinet\in.h>
#include <netdb.h>
#include <arpa\nameser.h>
#include <resolv.h>
#include <types.h>

#include "pop3.h"

#define	LOGFILE		"POP3.Log"	/* Name of log file */
#define	LOGENV		"ETC"		/* Environment variable for log dir */
#define	POP3DIR		"POP3"		/* Environment variable for spool dir */
#define	POP3SERVICE	"pop3"		/* Name of POP3 service */
#define	TCP		"tcp"		/* TCP protocol */

/* Type definitions */

typedef	struct hostent		HOST, *PHOST;		/* Host entry structure */
typedef struct in_addr		INADDR, *PINADDR;	/* Internet address */
typedef	struct sockaddr		SOCKG, *PSOCKG;		/* Generic structure */
typedef	struct sockaddr_in	SOCK, *PSOCK;		/* Internet structure */
typedef	struct servent		SERV, *PSERV;		/* Service structure */

/* Forward references */

static	VOID	fix_domain(PUCHAR);
static	VOID	log_connection(PUCHAR, BOOL);
static	VOID	process_logging(PUCHAR);
static	VOID	putusage(VOID);

/* Local storage */

static	LOGTYPE	log_type = LOGGING_UNSET;
static	PUCHAR	progname;		/* Name of program, as a string */
static	UCHAR	serverip[16];

/* Help text */

static	const	PUCHAR helpinfo[] = {
"%s: POP3 client",
"Synopsis: %s [options] ",
" Options:",
"    -bmsg        start with message number msg",
"    -ddirectory  specify directory to store mail",
"    -h           display this help",
"    -l           leave messages on server",
"    -nmax        limit number of messages retrieved to max",
"    -ppassword   specify password for mail collection",
"    -q           operate quietly",
"    -sserver     specify address of POP3 server",
"    -uusername   specify user for mail collection",
"    -v           verbose; display progress",
"    -zf          log to file (default)",
"    -zs          log to SYSLOG",
" ",
"If no directory is specified, a subdirectory of the directory described by",
"the environment variable "POP3DIR" is used. The subdirectory name is",
"the username, truncated if necessary to conform to file system limitations.",
"There is no default for the address of the POP3 server,",
"or for the username or password.",
""
};


/*
 * Parse arguments and handle options.
 *
 */

INT main(INT argc, PUCHAR argv[])
{	INT sockno, rc;
	INT i;
	BOOL leave = FALSE;
	BOOL verbose = FALSE;
	BOOL quiet = FALSE;
	BOOL dirspec;
	INT begin = INT_MAX;
	INT max = INT_MAX;
	PUCHAR argp, p;
	UCHAR servername[MAXHOSTNAMELEN+1];
	UCHAR clientname[MAXHOSTNAMELEN+1];
	UCHAR dirname[CCHMAXPATH+1];
	PUCHAR username = (PUCHAR) NULL;
	PUCHAR password = (PUCHAR) NULL;
	ULONG server_addr;
	SOCK server;
	PHOST pop3host;
	PSERV pop3serv;

	progname = strrchr(argv[0], '\\');
	if(progname != (PUCHAR) NULL)
		progname++;
	else
		progname = argv[0];
	p = strchr(progname, '.');
	if(p != (PUCHAR) NULL) *p = '\0';
	strlwr(progname);

	tzset();			/* Set time zone */
	res_init();			/* Initialise resolver */
	servername[0] = '\0';
	dirname[0] = '\0';

	/* Process input options */

	for(i = 1; i < argc; i++) {
		argp = argv[i];
		if(argp[0] == '-') {		/* Option */
			switch(argp[1]) {
				case 'b':	/* Specified beginning message */
					if(begin != INT_MAX) {
						error(
							"beginning message "
							"specified more than"
							" once");
						exit(EXIT_FAILURE);
					}
					if(argp[2] != '\0') {
						begin = atoi(&argp[2]);
					} else {
						if(i == argc - 1) {
							error("no arg for -b");
							exit(EXIT_FAILURE);
						} else {
							begin = atoi(argv[++i]);
						}
					}
					if(begin <= 0) {
						error("bad value for -b");
						exit(EXIT_FAILURE);
					}
					break;

				case 'd':	/* Specified directory */
					if(dirname[0] != '\0') {
						error(
							"directory specified "
							"more than once");
						exit(EXIT_FAILURE);
					}
					if(argp[2] != '\0') {
						strcpy(dirname, &argp[2]);
					} else {
						if(i == argc - 1) {
							error(
								"no arg for -d");
							exit(EXIT_FAILURE);
						} else {
							strcpy(
								dirname,
								argv[++i]);
						}
					}
					break;

				case 'h':	/* Display help */
					putusage();
					exit(EXIT_SUCCESS);

				case 'l':	/* Leave messages on server */
					leave = TRUE;
					break;

				case 'n':	/* Specified maximum messages */
					if(max != INT_MAX) {
						error(
							"maximum messages "
							"specified more than"
							" once");
						exit(EXIT_FAILURE);
					}
					if(argp[2] != '\0') {
						max = atoi(&argp[2]);
					} else {
						if(i == argc - 1) {
							error("no arg for -n");
							exit(EXIT_FAILURE);
						} else {
							max = atoi(argv[++i]);
						}
					}
					if(max <= 0) {
						error("bad value for -n");
						exit(EXIT_FAILURE);
					}
					break;

				case 'p':	/* Specified password */
					if(password != (PUCHAR) NULL) {
						error(
							"password specified "
							"more than once");
						exit(EXIT_FAILURE);
					}
					if(argp[2] != '\0') {
						password = xmalloc(strlen(&argp[2])+1);
						strcpy(password, &argp[2]);
					} else {
						if(i == argc - 1) {
							error("no arg for -p");
							exit(EXIT_FAILURE);
						} else {
							password = xmalloc(strlen(argv[++i])+1);
							strcpy(
								password,
								argv[i]);
						}
					}
					break;

				case 'q':	/* Quiet mode */
					quiet = TRUE;
					break;

				case 's':	/* Specified server name */
					if(servername[0] != '\0') {
						error(
							"server specified "
							"more than once");
						exit(EXIT_FAILURE);
					}
					if(argp[2] != '\0') {
						strcpy(servername, &argp[2]);
					} else {
						if(i == argc - 1) {
							error("no arg for -s");
							exit(EXIT_FAILURE);
						} else {
							strcpy(
								servername,
								argv[++i]);
						}
					}
					break;

				case 'u':	/* Specified user name */
					if(username != (PUCHAR) NULL) {
						error(
							"user name specified "
							"more than once");
						exit(EXIT_FAILURE);
					}
					if(argp[2] != '\0') {
						username = xmalloc(strlen(&argp[2])+1);
						strcpy(username, &argp[2]);
					} else {
						if(i == argc - 1) {
							error("no arg for -u");
							exit(EXIT_FAILURE);
						} else {
							username = xmalloc(strlen(argv[++i])+1);
							strcpy(
								username,
								argv[i]);
						}
					}
					break;

				case 'v':	/* Verbose - display progress */
					verbose = TRUE;
					break;

				case 'z':	/* Logging */
					if(log_type != LOGGING_UNSET) {
						error(
							"Logging option "
							"specified more than "
							"once");
						exit(EXIT_FAILURE);
					}
					if(argp[2] != '\0') {
						process_logging(&argp[2]);
					} else {
						if(i == argc - 1) {
							error ("no arg for -z");
							exit(EXIT_FAILURE);
						} else {
							i++;
							process_logging(argv[i]);
						}
					}
					break;

				case '\0':
					error("missing flag after '-'");
					exit(EXIT_FAILURE);

				default:
					error("invalid flag '%c'", argp[1]);
					exit(EXIT_FAILURE);
			}
		} else {
			error("invalid argument '%s'", argp);
			exit(EXIT_FAILURE);
		}
	}

	if(servername[0] == '\0') {
		error("server must be specified using -s");
		exit(EXIT_FAILURE);
	}
	fix_domain(servername);

	if(username == (PUCHAR) NULL) {
		error("user name must be specified using -u");
		exit(EXIT_FAILURE);
	}
	if(password == (PUCHAR) NULL) {
		error("password must be specified using -p");
		exit(EXIT_FAILURE);
	}

	if(dirname[0] == '\0') {
		PUCHAR dir = getenv(POP3DIR);
		PUCHAR temp;

		if(dir == (PUCHAR) NULL) {
			error(
				"no directory specified, and environment "
				"variable "POP3DIR" not set");
			exit(EXIT_FAILURE);
		}

		strcpy(dirname, dir);
		dirspec = FALSE;
	} else dirspec = TRUE;

	if(begin == INT_MAX) begin = 1;		/* Default starting message */

	/* Set default logging type if not specified */

	if(log_type == LOGGING_UNSET) log_type = LOGGING_FILE;

	/* Get the host name of this client; if not possible, set it to the
	   dotted address. */

	rc = gethostname(clientname, sizeof(clientname));
	if(rc != 0) {
		INADDR myaddr;

		myaddr.s_addr = htonl(gethostid());
		sprintf(clientname, "[%s]", inet_ntoa(myaddr));
	} else {
		fix_domain(clientname);
	}

	rc = sock_init();		/* Initialise socket library */
	if(rc != 0) {
		error("INET.SYS not running");
		exit(EXIT_FAILURE);
	}

	sockno = socket(PF_INET, SOCK_STREAM, 0);
	if(sockno == -1) {
		error("cannot create socket");
		exit(EXIT_FAILURE);
	}

	pop3host = gethostbyname(servername);
	if(pop3host == (PHOST) NULL) {
		if(isdigit(servername[0])) {
			server_addr = inet_addr(servername);
		} else {
			error(
				"cannot get address for POP3 server '%s'",
				servername);
			exit(EXIT_FAILURE);
		}
	} else {
		server_addr = *((u_long *) pop3host->h_addr);
	}

	pop3serv = getservbyname(POP3SERVICE, TCP);
	if(pop3serv == (PSERV) NULL) {
		error(
			"cannot get port for %s/%s service",
			POP3SERVICE,
			TCP);
		exit(EXIT_FAILURE);
	}
	endservent();

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = server_addr;
	server.sin_port = pop3serv->s_port;
	rc = connect(sockno, (PSOCKG) &server, sizeof(SOCK));
	if(rc == -1) {
		error("cannot connect to POP3 server '%s'", servername);
		exit(EXIT_FAILURE);
	}
	strcpy(serverip, inet_ntoa(server.sin_addr));

	/* Start logging */

	rc = open_log(log_type, LOGENV, LOGFILE, clientname, progname);
	if(rc != LOGERR_OK) {
		error(
		"logging initialisation failed - %s",
		rc == LOGERR_NOENV    ? "environment variable "LOGENV" not set" :
		rc == LOGERR_OPENFAIL ? "file open failed" :
					"internal log type failure");
		exit(EXIT_FAILURE);
	}

	log_connection(servername, quiet);
#ifdef	DEBUG
	trace("Server IP = %s", serverip);
#endif

	/* Do the work */

	rc = client(sockno, clientname, servername, serverip, dirname,
			username, password, dirspec, leave, verbose,
			begin, max);

	(VOID) soclose(sockno);
	close_log();

	return(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}


/*
 * Process the value of the '-z' option (logging).
 *
 */

static VOID process_logging(PUCHAR s)
{	if(strlen(s) == 1) {
		switch(toupper(s[0])) {
			case 'F':	/* Log to file */
				log_type = LOGGING_FILE;
				return;

			case 'S':	/* Log to SYSLOG */
				log_type = LOGGING_SYSLOG;
				return;
		}
	}
	error("invalid value for -z option");
	exit(EXIT_FAILURE);
}


/*
 * Check for a full domain name; if not present, add default domain name.
 *
 */

static VOID fix_domain(PUCHAR name)
{	if(strchr(name, '.') == (PUCHAR) NULL && _res.defdname[0] != '\0') {
		strcat(name, ".");
		strcat(name, _res.defdname);
	}
}


/*
 * Print message on standard error in printf style,
 * accompanied by program name.
 *
 */

VOID error(PUCHAR mes, ...)
{	va_list ap;

	fprintf(stderr, "%s: ", progname);

	va_start(ap, mes);
	vfprintf(stderr, mes, ap);
	va_end(ap);

	fputc('\n', stderr);
}


/*
 * Log details of the connection to standard output and to the logfile.
 *
 */

static VOID log_connection(PUCHAR servername, BOOL quiet)
{	time_t tod;
	UCHAR timeinfo[35];
	UCHAR buf[100];

	if(quiet == FALSE) {
		(VOID) time(&tod);
		(VOID) strftime(timeinfo, sizeof(timeinfo),
			"on %a %d %b %Y at %X %Z", localtime(&tod));
		sprintf(buf, "%s: connection to %s, %s",
			progname, servername, timeinfo);
			fprintf(stdout, "%s\n", buf);
	}

	sprintf(buf, "connection to %s", servername);
	dolog(LOG_INFO, buf);
}


/*
 * Output program usage information.
 *
 */

static VOID putusage(VOID)
{	PUCHAR *p = (PUCHAR *) helpinfo;
	PUCHAR q;

	for(;;) {
		q = *p++;
		if(*q == '\0') break;

		fprintf(stderr, q, progname);
		fputc('\n', stderr);
	}
	fprintf(stderr, "\nThis is version %d.%d\n", VERSION, EDIT);
}


/*
 * Allocate memory using 'malloc'; terminate with a message
 * if allocation failed.
 *
 */

PVOID xmalloc(size_t size)
{	PVOID res;

	res = malloc(size);

	if(res == (PVOID) NULL)
		error("cannot allocate memory");

	return(res);
}

/*
 * End of file: pop3.c
 *
 */

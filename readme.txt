POP3 - a simple POP3 client for OS/2
------------------------------------

This is a very simple and basic POP3 client, which will collect mail
messages from a POP3 server and store them (one per file) in a specified
directory.  It works best when the directory is on an HPFS partition. 
It is not sophisticated, but it is small and fast. 

All companion programs are available from the same place:

     http://www.tavi.co.uk/os2pages/mail.html

Installation
------------

Copy the POP3.EXE file to any suitable directory which is named in the
system PATH (actually, this isn't strictly necessary but it can make
life easier).  Copy the NETLIB.DLL file to any directory on the LIBPATH. 

Configuration
-------------

First, ensure that you have a line in CONFIG.SYS of the form:

     SET TZ=....

This defines your time zone setting, names, and daylight saving rules. 
If you don't have one, you need to add it; the actual value can be quite
complex.  For the United Kingdom, the line is:

     SET TZ=GMT0BST,3,-1,0,7200,10,-1,0,7200,3600

For other areas, download the TZCALC utility from the same place as this
program.  This will work out the correct setting for you.  It will be
necessary to reboot in order to pick up this setting, but wait until you
have completed the rest of these instructions.

Now edit CONFIG.SYS, adding a new line of the form:

     SET POP3=directoryname

where 'directoryname' is the name of a directory (which must exist)
where incoming mail is stored.  You will also need to create a
subdirectory for each user for whom mail is to be collected.  The name
of the directory should be the same as the username on the POP3 server
you are contacting.  If you are using a FAT partition, then the first 11
characters of the username are used, forced to upper case (if there are
illegal characters in the username then this fails).  If you have
problems, it is possible to specify a particular directory on the
command line, so don't worry.  All in all, though, HPFS is the best
solution. 

It will be necessary to reboot in order to pick up the POP3 setting, but
wait until you have completed the rest of these instructions. 

Locate the directory described by the ETC environment variable.  If you
are not sure, type the command:

     SET ETC

at a command prompt.  You need take no action, but note that this is
where the logfile (see below) is stored. 

Logfile
-------

If the -zf option is used (or neither it, nor the -zs option are used),
POP3 maintains a logfile called POP3.LOG in the \MPTN\ETC directory. 
This will grow without bound if not pruned regularly!

If the -zs option is used, then the log information is sent instead to
the SYSLOG daemon, if it is running.  Normally, this sends the output to
the file SYSLOG.MSG in the \MPTN\ETC directory, although this behaviour
can be altered by editing the file \MPTN\ETC\SYSLOG.CNF (this file
contains comments which explain what to edit).  Unfortunately,
SYSLOG.MSG is locked open by the SYSLOG daemon, so to read its contents
it is easiest to use the command sequence:

     TYPE SYSLOG.MSG > Z
     E Z                     (or use any other program to view the file Z)

Using the program
-----------------

To connect to a POP3 server and retrieve all pending messages, simply
run the program.  The following command line flags are recognised:

        -b msg	start with message number msg
	-h	Display a brief help message
	-d dir	Specify the name of directory where mail is to be stored
	-l	collect mail, but don't delete it from the server
	-s srv	Specify the name of the POP3 server
	-v	Turn on verbose mode (extra advisory messages)
	-n max	Limit the number of retrieved messages to 'num'. Useful if
		there is a flood of mail.
	-u user	Specify the username on the POP3 server
	-p pass	Specify the password on the POP3 server
        -zf     Log to file (default)
	-zs	Log to SYSLOG

For example, to retrieve mail from the server abc.xyz.net, with verbose
mode on:

	pop3 -v -sabc.xyz.net -ufred -pbloggs

That's all!

Feedback
--------

POP3 was written by me, Bob Eager. I can be contacted at rde@tavi.co.uk.

Please let me know if you actually use this program.  Suggestions for
improvements are welcomed. 

History
-------

1.0	Initial version.
1.1	Better handling of long usernames/passwords.
	Correction to generation of mail directory based on
	username on FAT volumes.
1.2	Added -n flag to limit number of messages retrieved.
1.3	Added Received: line to incoming mail messages.
1.4	Fixed problem with non fully-qualified receive directory
	names.
1.5	Fixed problem with reversed source and destination names
	in generated Received: header line.
1.6	Added -v option to display progress of mail collection.
1.7	Use new thread-safe logging module.
	Use OS/2 type definitions.
	New, simplified network interface module.
1.8	Added option security logging.
1.9	Added final message count if verbose option turned on.
	Modified to use NETLIB DLL.
2.0	Added BLDLEVEL string.
	Diagnostics for occasional logfile open failures.
	Additional error checking in logging module.
	Grouped initialisation code together.
	Added -b flag to allow retrieval to start at a particular
	message.
3.0	Recompiled using 32-bit TCP/IP toolkit, in 16-bit mode.
4.0	Added support for logging to 'syslog' instead of to file,
	selectable by '-z' option. This has the advantage of
	avoiding clashing logfile usage.
	Changes to conform more closely with RFC 2821.
	Add client IP to Received: line.
	Changed timezone to offset in Received: line, to comply
	with RFC2821.
	Make timestamps conform to RFC2821/RFC2822 in terms of
	leading zeros and four digit year values.
4.1	Added -q flag for quiet operation.

Bob Eager
rde@tavi.co.uk
December 2003

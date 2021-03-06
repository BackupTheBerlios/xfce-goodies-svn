/*
 *  Generic Monitor plugin for the Xfce4 panel
 *  Spawn - Spawn a process and capture its output
 *  Copyright (c) 2004 Roger Seguin <roger_seguin@msn.com>
 *  					<http://rmlx.dyndns.org>
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.

 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

static char     RCSid[] =
    "$Id: cmdspawn.c,v 1.1 2004/09/09 13:35:51 rogerms Exp $";


#define DEBUG	0

	/* Posix-compliance to make sure that only the calling thread is
	   duplicated, not the whole process (e.g Solaris) */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE	199506L
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE	500
#endif

#include "cmdspawn.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <strings.h>
#include <string.h>
#include <poll.h>
#include <errno.h>


/**********************************************************************/
int genmon_Spawn (char *const argv[], char *const p_pcOutput,
		  const size_t p_BufferSize)
/**********************************************************************/
	/* Spawn a command and capture its output */
	/* Return 0 on success, otherwise copy stderr into the output
	   string and return -1 */
{
    enum { OUT, ERR, OUT_ERR };
    enum { RD, WR, RD_WR };
    const size_t    BufSafeSize = p_BufferSize - 1;
    // Make sure that the output string will be NULL-terminated
    int             aaiPipe[OUT_ERR][RD_WR];
    pid_t           pid;
    struct pollfd   aoPoll[OUT_ERR];
    int             fError;
    int             status;
    int             i, j, k;

    if (p_BufferSize <= 0) {
	fprintf (stderr, "Spawn() error: Wrong buffer size!\n");
	return (-1);
    }
    memset (p_pcOutput, 0, p_BufferSize);
    if (!(*argv)) {
	strncpy (p_pcOutput, "Spawn() error: No parameters passed!",
		 BufSafeSize);
	return (-1);
    }
    for (i = 0; i < OUT_ERR; i++)
	pipe (aaiPipe[i]);
    switch (pid = fork ()) {
	case -1:
	    i = errno;
	    snprintf (p_pcOutput, BufSafeSize, "fork(%d): %s", i,
		      strerror (i));
	    for (i = 0; i < OUT_ERR; i++)
		for (j = 0; j < RD_WR; j++)
		    close (aaiPipe[i][j]);
	    return (-1);
	case 0:
	    /* Redirect stdout/stderr to associated pipe's write-ends */
	    for (i = 0; i < OUT_ERR; i++) {
		j = i + 1;	// stdout/stderr file descriptor
		close (j);
		k = dup2 (aaiPipe[i][WR], j);
		if (k != j) {
		    perror ("dup2()");
		    exit (-1);
		}
	    }

	    /* Execute the given command */
	    execvp (argv[0], argv);
	    perror (argv[0]);
	    exit (-1);
    }

    /* Wait for child completion */
    status = waitpid (pid, 0, 0);
    if (status == -1) {
	i = errno;
	snprintf (p_pcOutput, BufSafeSize, "waitpid(%d): %s", i,
		  strerror (i));
	fError = 1;
	goto End;
    }

    /* Read stdout/stderr pipes' read-ends */
    for (i = 0; i < OUT_ERR; i++) {
	aoPoll[i].fd = aaiPipe[i][RD];
	aoPoll[i].events = POLLIN;
	aoPoll[i].revents = 0;
    }
    poll (aoPoll, OUT_ERR, ~0);
    for (i = 0; i < OUT_ERR; i++)
	if (aoPoll[i].revents & POLLIN)
	    break;
    if (i < OUT_ERR)
	read (aaiPipe[i][RD], p_pcOutput, BufSafeSize);
    fError = (i != OUT);

    /* Remove trailing carriage return if any */
    if (p_pcOutput[(i = strlen (p_pcOutput) - 1)] == '\n')
	p_pcOutput[i] = 0;

  End:
    /* Close created pipes */
    for (i = 0; i < OUT_ERR; i++)
	for (j = 0; j < RD_WR; j++)
	    close (aaiPipe[i][j]);

    return (-fError);
}				// Spawn()


/**********************************************************************/
int genmon_SpawnCmd (const char *const p_pcCmdLine, char *const p_pcOutput,
		     const size_t p_BufferSize)
/**********************************************************************/
	/* Spawn a command and capture its output */
	/* Return 0 on success, otherwise copy stderr into the output
	   string and return -1 */
{
    const size_t    N = strlen (p_pcCmdLine) + 1;
    const size_t    BufSafeSize = p_BufferSize - 1;
    char            acFormat[16];
    char           *pcStr, *pcStr1, *pcStr2;
    char          **argv;
    int             argc;
    int             n;
    int             status;

    pcStr = (char *) malloc (N);
    pcStr1 = (char *) malloc (N);
    pcStr2 = (char *) malloc (N);
    argv = (char **) malloc (N);
    if (!(pcStr && pcStr1 && pcStr2 && argv)) {
	n = errno;
	snprintf (p_pcOutput, BufSafeSize, "malloc(%d): %s", n,
		  strerror (n));
	return (-1);
    }

    /* Build argv from the command line string */
    sprintf (acFormat, "%%s %%%dc", N - 1);
    strcpy (pcStr, p_pcCmdLine);
    for (argc = 0;;) {
	memset (pcStr2, 0, N);
	n = sscanf (pcStr, acFormat, pcStr1, pcStr2);
	if (n <= 0)
	    break;
	argv[argc] = (char *) malloc (strlen (pcStr1) + 1);
	if (!(argv[argc])) {
	    n = errno;
	    snprintf (p_pcOutput, BufSafeSize, "malloc(%d): %s", n,
		      strerror (n));
	    free (pcStr), free (pcStr1), free (pcStr2);
	    while (argc-- > 0)
		free (argv[argc]);
	    free (argv);
	    return (-1);
	}
	strcpy (argv[argc++], pcStr1);
	if (n <= 1)
	    break;
	strcpy (pcStr, pcStr2);
    }
    free (pcStr), free (pcStr1), free (pcStr2);

    /* Spawn the command and free allocated memory */
    argv[argc] = 0;
    status = genmon_Spawn (argv, p_pcOutput, p_BufferSize);
    while (argc-- > 0)
	free (argv[argc]);
    free (argv);
    return (status);
}				// SpawnCmd()


	/**************************************************************/

/*
$Log: cmdspawn.c,v $
Revision 1.1  2004/09/09 13:35:51  rogerms
Initial revision

Revision 1.2  2004/08/28 13:33:27  RogerSeguin
Better processing

Revision 1.1  2004/08/27 23:17:19  RogerSeguin
Initial revision

*/

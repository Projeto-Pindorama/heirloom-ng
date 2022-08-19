/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "add_recip.c	1.7	05/06/08 SMI"	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)add_recip.c	1.5 (gritter) 7/3/05
 */
	 	/* SVr4.0 1.5	*/
/*
    NAME
	add_recip, madd_recip - add recipients to recipient list

    SYNOPSIS
	int add_recip(reciplist *plist, char *name, int checkdups)
	int madd_recip(reciplist *plist, char *name, int checkdups)

    DESCRIPTION
	add_recip() adds the name to the recipient linked list.
	If checkdups is set, it first checks to make certain that
	the name is not in the list.

	madd_recips() is given a list of names separated by white
	space. Each name is split off and passed to add_recips.
*/

#include "mail.h"
#include "asciitype.h"

int
add_recip(reciplist *plist, char *name, int checkdups)
{
	char		*p;
	static char	pn[] = "add_recip";
	recip		*r = &plist->recip_list;

	if ((name == (char *)NULL) || (*name == '\0')) {
		Tout(pn, "translation to NULL name ignored\n");
		return(0);
	}

	p = name;
	while (*p && !spacechar(*p&0377)) {
		p++;
	}
	if (*p != '\0') {
	    Tout(pn, "'%s' not added due to imbedded spaces\n", name);
	    return(0);
	}

	if (checkdups == TRUE) {
	    while (r->next != (struct recip *)NULL) {
		r = r->next;
		if (strcmp(r->name, name) == 0) {
			Tout(pn, "duplicate recipient '%s' not added to list\n",
									name);
			return(0);
		}
	    }
	}

	if ((p = malloc (sizeof(struct recip))) == (char *)NULL) {
		errmsg(E_MEM,"first malloc failed in add_recip()");
		done(1);
	}
	plist->last_recip->next = (struct recip *)p;
	r = plist->last_recip = plist->last_recip->next;
	if ((r->name = malloc (strlen(name)+1)) == (char *)NULL) {
		errmsg(E_MEM,"second malloc failed in add_recip()");
		done(1);
	}
	strcpy (r->name, name);
	r->next = (struct recip *)NULL;
	Tout(pn, "'%s' added to recipient list\n", name);

	return(1);
}

void
madd_recip(reciplist *plist, char *namelist, int checkdups)
{
	char	*name;
	for (name = strtok(namelist, " \t"); name; name = strtok((char*)0, " \t"))
		add_recip(plist, name, checkdups);
}

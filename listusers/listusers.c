/*
 * System V listusers command lookalike.
 *
 *   Copyright (c) 2000 Gunnar Ritter. All Rights Reserved.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)listusers.sl	1.13 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<grp.h>
#include	<libgen.h>

/*
 * Any login whose uid is below this value is ignored.
 */
#define	MIN_USER_UID	100

struct list {
	struct list	*next;		/* next item in list */
	char		*name;		/* login name */
	char		*gecos;		/* gecos field */
};

struct grid {
	struct grid	*next;		/* next item in list */
	gid_t		gid;		/* numerical group information */
};

char		*progname;		/* arg0 to main */

#define	CATSET	5			/* messages catalogue set */

/*
 * Memory allocation with check.
 */
void *
smalloc(size_t n)
{
	void *s = malloc(n);
	if (s)
		return s;
	write(2, "Internal space allocation error.  Command terminates\n", 53);
	exit(077);
}

/*
 * Add a name and a gecos description to the end of a list.
 * If the list had no members yet, return its beginning, else NULL.
 */
struct list *
addlist(struct list *l, char *name, char *gecos)
{
	struct list *n = (struct list *)smalloc(sizeof(struct list));
	n->name = (char *)smalloc(strlen(name) + 1);
	strcpy(n->name, name);
	n->gecos = (char *)smalloc(strlen(gecos) + 1);
	strcpy(n->gecos, gecos);
	n->next = NULL;
	if (l) {
		while (l->next)
			l = l->next;
		l->next = n;
		return 0;
	} else
		return n;
}

/*
 * Convert a comma-separated string of names to a list.
 */
struct list *
userlist(char *s)
{
	char *c;
	struct list *l, *u = NULL;

	do {
		if (c = strchr(s, ','))
			*c = '\0';
		if (l = addlist(u, s, ""))
			u = l;
		s = c + 1;
	} while (c);
	return u;
}

/*
 * Add an entry to numerical group list.
 */
void
addgrid(struct grid **gi, gid_t gid)
{
	struct grid *g;

	if (*gi != NULL) {
		for (g = *gi; g->next; g = g->next);
		g->next = (struct grid *)smalloc(sizeof *g);
		g = g->next;
	} else
		g = *gi = (struct grid *)smalloc(sizeof *g);
	g->next = NULL;
	g->gid = gid;
}

/*
 * Check whether an users's gid matches the gid list.
 */
int
hasgrid(struct grid *g, gid_t gid)
{
	if (g == NULL)
		return 1;
	while (g) {
		if (g->gid == gid)
			return 1;
		g = g->next;
	}
	return 0;
}

/*
 * Get a comma-separated group string and build a list of all users
 * who belong to them.
 */
struct list *
grouplist(char *s, struct grid **gi)
{
	char *c;
	struct list *l, *u = NULL;
	struct group *grp;
	int i;

	do {
		if (c = strchr(s, ','))
			*c = '\0';
		if (grp = getgrnam(s)) {
			addgrid(gi, grp->gr_gid);
			for (i = 0; grp->gr_mem[i]; i++)
				if (l = addlist(u, grp->gr_mem[i], ""))
					u = l;
		}
	} while (c);
	if (u == NULL)
		u = addlist(u, "", "");
	return u;
}

/*
 * Check whether a string is contained within a list.
 */
int
matchlist(struct list *l, char *s)
{
	if (l == NULL)
		return 1;
	do
		if (strcmp(l->name, s) == 0)
			return 1;
	while (l = l->next);
	return 0;
}

/*
 * Compare list members for sorting.
 */
int
lcompar(const void *a, const void *b)
{
	return strcmp((*(struct list **)a)->name, (*(struct list **)b)->name);
}

/*
 * Print a list, sorted by names.
 */
void
printlist(struct list *l)
{
	int i, lcount;
	struct list **n, *m;
	char *prev = NULL;

	if (l == NULL)
		return;
	for (lcount = 1, m = l; m->next; m = m->next, lcount++);
	n = (struct list **)smalloc(sizeof(struct list *) * lcount);
	for (i = 0, m = l; i < lcount; i++, m = m->next)
		n[i] = m;
	qsort(n, lcount, sizeof(struct list *), lcompar);
	for (i = 0; i < lcount; i++) {
		if (prev == NULL || strcmp(n[i]->name, prev))
			printf("%-16s%s\n", n[i]->name, n[i]->gecos);
		prev = n[i]->name;
	}
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-g groups] [-l logins]\n", progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	const char optstring[] = ":g:l:";
	int i;
	struct list *groups = NULL, *logins = NULL, *users = NULL, *l;
	struct passwd *pwd;
	struct grid *gi = NULL;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'g':
			groups = grouplist(optarg, &gi);
			break;
		case 'l':
			logins = userlist(optarg);
			break;
		default:
			usage();
		}
	}
	if (argv[optind])
		usage();
	setpwent();
	while (pwd = getpwent())
		if (pwd->pw_uid >= MIN_USER_UID
				&& matchlist(logins, pwd->pw_name)
				&& (matchlist(groups, pwd->pw_name)
					|| hasgrid(gi, pwd->pw_gid)))
			if (l = addlist(users, pwd->pw_name, pwd->pw_gecos))
				users = l;
	printlist(users);
	return 0;
}

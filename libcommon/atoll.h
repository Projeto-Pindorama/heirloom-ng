/*	Sccsid @(#)atoll.h	1.4 (gritter) 7/18/04	*/

#if defined (__hpux) || defined (_AIX) || \
	defined (__FreeBSD__) && (__FreeBSD__) < 5
extern long long strtoll(const char *nptr, char **endptr, int base);
extern unsigned long long strtoull(const char *nptr, char **endptr, int base);
extern long long atoll(const char *nptr);
#endif	/* __hpux || _AIX || __FreeBSD__ < 5 */

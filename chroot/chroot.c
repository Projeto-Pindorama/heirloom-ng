#include <errno.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_error(int error) {
	errno = 0;
	char *str = strerror(error);
	if (errno != 0)
		pfmt(stderr, MM_ERROR, "errno %d\n", error);
	else
		pfmt(stderr, MM_ERROR, "%s\n", str);
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <newroot> <command>\n", argv[0]);
		exit(1);
	}

	// If it's not running as su
	//if (geteuid() != 0) {
//		pfmt(stderr, MM_ERROR, "not running as su.\n");
		//exit(2);
	//}

	if (
		chroot(argv[1]) < 0 ||
		chdir("/") < 0 ||
		execv(argv[2], &argv[2]) < 0
	) {
		print_error(errno);
		exit(3);
	}	
}

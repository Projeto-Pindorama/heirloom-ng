#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ROOT_UID 0

void print_error(int error) {
	errno = 0;
	char *str = strerror(error);
	if (errno != 0)
		fprintf(stderr, "Errno: %d\n", error);
	else
		fprintf(stderr, "%s\n", str);
}

int main(int argc, char *argv[]) {
	if (argc < 3){
		fprintf(stderr, "Usage: %s <newroot> <command>\n", argv[0]);
		exit(1);
	}

	if (geteuid() != ROOT_UID){
		fputs("Please run as su\n", stderr);
		exit(2);
	}


	if (chroot(argv[1]) < 0 || chdir("/") < 0){
		print_error(errno);
		exit(3);
	}	

	if (execv(argv[2], &argv[2]) < 0){
		print_error(errno);
		exit(3);
	}
}

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
	// ./more <arquivo>
	if (argc != 2) {
		printf("Uso: %s <arquivo>\n", argv[0]);
		return 0;
	}

	int fd;
	fd = open(argv[1], O_RDONLY, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		perror("open: ");
		return 0;
	}

	#define BLK 4000
	char buf[BLK];
	int n;
	do {
		n = read(fd, buf, BLK);
		if (n > 0) {
			printf("%s", buf);
			getchar();
		}
	} while(n>0);
	close(fd);
	return 0;
}
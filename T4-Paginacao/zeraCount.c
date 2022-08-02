#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv){
    int a, fdAux;
    char b[1], out[10];
    b[0]=0;//update count
    fdAux = open("count.txt", O_WRONLY, S_IRUSR | S_IWUSR);
    if (fdAux == -1) { //erro de abertura
        perror("open: ");
        exit(1);
	}
    write(fdAux, b, 1);//updating the count
    close(fdAux);
}
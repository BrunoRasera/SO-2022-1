#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define READ_END 0 //indice extremo do pipe para leitura
#define WRITE_END 1 //indice extremo do pipe para escrita

int main(){
    int fd1[2]; //descritor
    int status, pid;

    pipe(fd1); //comunica que existe comandos

    pid = fork(); //cria um filho

    if(pid == 0){ //filho 1 que executa ls-l
        close(fd1[READ_END]); //fecha o extremo de leitura que não está sendo usado
        dup2(fd1[WRITE_END], STDOUT_FILENO); //copia a saida para o descritor
        close(fd1[WRITE_END]); //fecha o extremo de escrita
        execlp ("/bin/ls", "ls", "-l", NULL); //executa o comando
    }else{ //pai
        close(fd1[WRITE_END]); //fecha o extremo de escrita que não está sendo usado
        pid = fork(); //cria um filho
        if(pid == 0){ //filho 2 que executa wc
            dup2(fd1[READ_END], STDIN_FILENO);
            close(fd1[READ_END]); //fecha o extremo de leitura que não está sendo usado
            execlp("/usr/bin/wc", "wc", NULL);//executa o comando
        }else{ //pai
            close(fd1[READ_END]); //fecha o extremo de leitura que não está sendo usado
        }
    }
    //um wait para cada filho
    wait(&status);
    wait(&status);
    return 0;
}
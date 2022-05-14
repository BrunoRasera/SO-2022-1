/*
Projeto 1: Shell 
--------------------------
Aluno: Bruno Rasera
   RA: 143196
Aluno: Daniel de Souza Paiva
   RA: 143196
Aluno: Letícia A. P. Lisboa
   RA: 139969
Aluno: Mateus Gomes Ferreira
   RA: 140886
Os comandos devem ser utilizados entre aspas. Ex: ./a.out ls -la "|" grep teste
tipo 0 => |
tipo 1 => || (OR)
tipo 2 => && (AND)
tipo 3 => &
Exemplos de comandos:
./a.out ls -la
./a.out ls -la "|" grep teste
./a.out ls -la "|" grep teste "|" wc -l
./a.out echo "Mensagem 1" "&&" ls 
./a.out echo "Mensagem 1" "&&" ls "&&" ls -la "|" grep teste
./a.out echo "Mensagem 1" "||" echo "Mensagem 2" "||" echo "Mensagem 3"
./a.out eco "Mensagem 1" "||" echo "Mensagem 2" "||" echo "Mensagem 3"
./a.out eco "Mensagem 1" "||" eco "Mensagem 2" "||" echo "Mensagem 3"
./a.out ping -c1 www.unifesp.br "&&" echo "SERVIDOR DISPONIVEL" "||" echo "SERVIDOR INDISPONIVEL"
./a.out ping -c5 www.google.com "&"
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/*  Função auxiliar que conta a quantidade de 
    comandos e operadores enviados 
    e salva nas posições 0 e 1 de qtds respectivamente
*/
void qtdBlocosOps (int argc, char **cmd, int *qtds){

    int i;

    /*  Flag para leitura de comandos, 
        quando 0 indica que ainda estamos lendo o mesmo comando, 
        quando 1 estamos lendo um comando novo
    */ 
    int cont = 1;

    for(i = 0; i < argc - 1; i++){

        // Novo comando
        if((strcmp(cmd[i],"|") != 0 || strcmp(cmd[i],"||") != 0 || strcmp(cmd[i],"&&") != 0 || strcmp(cmd[i],"&") != 0 ) && cont == 1){
            qtds[0] += 1;
            cont = 0;

        }
        else if (strcmp(cmd[i],"|") == 0 || strcmp(cmd[i],"||") == 0 || strcmp(cmd[i],"&&") == 0 || strcmp(cmd[i],"&") == 0 ) {

            qtds[1] += 1;
            cont = 1;

        }
    }
}


int main(int argc, char **argv){
    pid_t pid;

    // Matriz de comandos recebe argv menos a primeira posição (chamada do binário do shell)
    char **cmd = &argv[1];

    /*
        Vetor para contar a quantidade de comandos e operadores,
        posição 0 para comandos e 1 para operadores
    */
    int qtds[2] = {0,0};

    qtdBlocosOps(argc, cmd, qtds);

    // Vetor utilizado para armazenar as posições onde cada bloco de comando começa na matriz cmd
    int blocos[qtds[0]];
    blocos[0] = 0;

    // **fd é usado para controlar os pipes, que serão alocados futuramente
    int **fd;

    // Representa a quantidade de processos filhos que serão criados
    int filhos = 1;

    // Vetor que armazena os tipos de operadores
    int tipo[qtds[1]];

    int i, k = 0;

    // Checa se o programa foi executado com o número correto de argumentos
    if (argc == 1){

        printf("Uso correto: %s <comando> <p_1> <p_2> ... <p_n>\n", argv[0]);
        return 0;

    }

    // Checamos os tipos de comandos existentes e separamos em blocos 
    for(i = 0; i < argc - 1; i++){
        // Caso pipe ("|")
        if(strcmp(cmd[i],"|") == 0){
            tipo[k] = 0;
            tipo[k+1] = 0;
            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k = k + 2;
        }

        // Caso OR ("||")
        else if(strcmp(cmd[i], "||") == 0){
            tipo[k] = 1;
            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k ++;
        }

        // Caso AND ("&&")
        else if(strcmp(cmd[i], "&&") == 0){
            tipo[k] = 2;

            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k ++;
        }

        // Caso background ("&")
        else if(strcmp(cmd[i], "&") == 0){
            tipo[k] = 3;
            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k++;
        }

    }

    // Alocação da matriz de pipes
    fd = (int**) malloc((filhos)*sizeof(int*));

    for(i=0; i<filhos; i++){
        fd[i] = (int*) malloc(2*sizeof(int));
        pipe(fd[i]);

    }

    // Itera sobre blocos e tipo usando a quantidade de filhos/forks que serão necessários
    for(i = 0; i < filhos; i++){
        int status;
        pid = fork();

        // Processo filho
        if(pid == 0){
            // Pipe
            if (tipo[i] == 0){
                if(i != 0){
                    close(fd[i-1][1]);
                    dup2(fd[i-1][0], STDIN_FILENO);
                    close(fd[i-1][0]);
                }

                // Condições que a saída nao deve ser redirecionada pro terminal como texto
                if(i != filhos-1 && tipo[i+1] == 0){
                    close(fd[i][0]);
                    dup2(fd[i][1], STDOUT_FILENO);
                    close(fd[i][1]);
                }

                // Retorna -1 caso o comando falhe 
                int resultado = execvp(cmd[blocos[i]], &cmd[blocos[i]]);
                if (resultado == -1)
                    exit(-1);
                // Retorna 0 caso tenha êxito
                exit(0);
            }
            // Demais casos (background, OR e AND)
            else{
                int resultado = execvp(cmd[blocos[i]], &cmd[blocos[i]]);

                if (resultado == -1)
                    exit(-1);
                exit(0);
            }
        }

        // Processo pai
        else if (pid > 0){

            // Fecha o lado do pipe que não vai ser necessário
            if(tipo[i] == 0)
                close(fd[i][1]);
            
            // Espera o processo filho terminar exceto para background
            if(tipo[i] == 3) waitpid(pid, &status, WNOHANG);
            else waitpid(pid, &status, 0);

            // OR
            if(tipo[i] == 1){
                /* 
                    Se o comando foi executado com sucesso, 
                    interrompe a execução dos demais comandos OR
                */
                if(status == 0){

                    while(tipo[i] == 1 && i < filhos -1) i = i + 1;

                }
            }

            // AND
            else if(tipo[i] == 2){
                /* 
                    Se o comando falhou na execução,
                    interrompe a execução dos demais comandos AND
                */
                if(status != 0){
                    while(tipo[i] == 2 && i < filhos -1) i = i + 1;
                }
            }
            
        }

        // Erro na criação do fork()
        else printf("Erro no fork()");

    }

    // Liberando os espaços alocados
    for(i=0; i<filhos; i++) free(fd[i]);

    free(fd);

    return 0;
}
/*
IMPLEMENTAÇÃO DE UM SHELL
--------------------------

Aluno: Bruno Rasera
   RA: 143196
Aluno: Leonardo Silva Pinto
   RA: 133732


Os comandos devem ser utilizados entre aspas. Ex: ./a.out ls -la "|" grep teste

tipo 0 => |
tipo 1 => ;
tipo 2 => || (OR)
tipo 3 => && (AND)
tipo 4 => &

Exemplos de comandos:

./a.out ls -la
./a.out ls -la "|" grep teste
./a.out ls -la "|" grep teste "|" wc -l

./a.out echo "SO 2021" ";" echo "ADE Viva"
./a.out echo "SO 2021" ";" echo "ADE Viva" ";" echo "Unifesp"
./a.out echo "SO 2021" ";" eco "ADE Viva" ";" echo "Unifesp"

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


int main(int argc, char **argv)
{
    pid_t pid;

    // Matriz de comandos recebe argv menos a primeira posição
    char **cmd = &argv[1];

    // Vetor utilizado para armazenar as posições onde cada bloco de comando começa
    int blocos[10];
    blocos[0] = 0;

    // **fd é usado para controlar os pipes, que serão alocados futuramente
    int **fd;

    // Representa a quantidade de processos filhos que serão criados
    int filhos = 1;

    // Vetor para controlar os tipos de operadores
    int tipo[10];

    int i, k=0;

    // Checa se o programa foi executado com o número correto de argumentos
    if (argc == 1)
    {
        printf("Uso correto: %s <comando> <p_1> <p_2> ... <p_n>\n", argv[0]);
        return 0;
    }

    // Checamos os tipos de comandos existentes e separamos em blocos 
    for(i=0; i<argc-1; i++)
    {
        // Caso pipe ("|")
        if(strcmp(cmd[i],"|") == 0)
        {
            tipo[k] = 0;
            tipo[k+1] = 0;
            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k = k + 2;
        }
        // Caso independente (;)
        else if(strcmp(cmd[i], ";") == 0)
        {
            tipo[k] = 1;
            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k++;
        }
        // Caso OR ("||")
        else if(strcmp(cmd[i], "||") == 0)
        {
            tipo[k] = 2;
            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k ++;
        }
        // Caso AND ("&&")
        else if(strcmp(cmd[i], "&&") == 0)
        {
            tipo[k] = 3;

            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k ++;
        }
        // Caso background ("&")
        else if(strcmp(cmd[i], "&") == 0)
        {
            tipo[k] = 4;
            blocos[filhos] = i+1;
            cmd[i] = NULL;
            filhos++;
            k++;
        }

    }

    // Alocação da matriz de pipes
    fd = (int**) malloc((filhos)*sizeof(int*));
    for(i=0; i<filhos; i++)
    {
        fd[i] = (int*) malloc(2*sizeof(int));
        pipe(fd[i]);
    }

    // Variável de controle para parar a execução caso necessário
    int continua_execucao = 1;

    // Roda pela quantidade de filhos/forks que serão necessários
    for(i=0; i<filhos; i++)
    {
        int status;
        pid = fork();

        // Filho
        if(pid == 0)
        {
            // Pipe
            if (tipo[i] == 0)
            {
                if(i != 0)
                {
                    close(fd[i-1][1]);
                    dup2(fd[i-1][0], STDIN_FILENO);
                    close(fd[i-1][0]);
                }
                // Condições que a saída nao deve ser redirecionada pro terminal como texto
                if(i != filhos-1 && tipo[i+1] == 0) 
                {
                    close(fd[i][0]);
                    dup2(fd[i][1], STDOUT_FILENO);
                    close(fd[i][1]);
                }
                // Retorna -1 caso o comando falhe 
                int resultado = execvp(cmd[blocos[i]], &cmd[blocos[i]]);
                if (resultado == -1)
                    exit(-1);
                exit(0);
            }
            // Background
            else if (tipo[i] == 4)
            {
		int resultado = execvp(cmd[blocos[i]], &cmd[blocos[i]]);
                //execlp ("/bin/bg", "bg", getpid(), NULL);

                if (resultado == -1)
                    exit(-1);
                exit(0);
            }
            // Demais casos
            else
            {
                int resultado = execvp(cmd[blocos[i]], &cmd[blocos[i]]);
                if (resultado == -1)
                    exit(-1);
                exit(0);
            }
        }

        // Pai
        else if (pid > 0)
        {
            // Fecha o lado do pipe que não vai ser necessário
            if(tipo[i] == 0)
                close(fd[i][1]);
            
            // Espera o processo filho terminar
            if(tipo[i] == 4)
            	waitpid(pid, &status, WNOHANG);
	    else 
	    	waitpid(pid, &status, 0);
            // Caso OR. Se o comando foi executado com sucesso, 
            // interrompe a execução dos demais comandos
            if(tipo[i] == 2)
            {
                if(status == 0)
                    continua_execucao = 0;
            }

            // Caso AND. Se o comando falhou na execução, 
            // interrompe a execução dos demais comandos
            if(tipo[i] == 3)
            {
                if(status != 0)
                    continua_execucao = 0;
            }
            
        }

        // Erro na criação do fork()
        else
            printf("Erro no fork()");

        // Interrompe a execução de acordo com o caso OR ou AND
        if(continua_execucao == 0)
            break;
    }

    // Liberando espaço alocado
    for(i=0; i<filhos; i++)
    {
        free(fd[i]);
    }
    free(fd);

    return 0;
}

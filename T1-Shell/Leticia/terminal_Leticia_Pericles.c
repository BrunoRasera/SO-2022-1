#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void ler_teclado(char s[]){
	printf("> ");
	scanf("%[^\n]s", s);

	getchar();

	int i, j, tamanho = strlen(s);
	int flag_aspas = 0;

	// ter sempre um espaço depois das palavras
	s[tamanho] = ' ';
	s[tamanho + 1] = '\0';
	tamanho++;
	// evitar espaços desnecessários, sempre depois de uma palavra ter somente um espaço

	for(j = 0; j < tamanho; j++){
		// achar primeira letra
		if(s[j] != ' '){
			break;
		}
	}
	// i eh iterador para a string de entrada com somente um espaco depois das palavras
	// j eh o iterador da string de entrada
	for(i = 0; s[j] != '\0';){
		if(s[j] == '"'){
			j++;
			while(s[j] != '"' && s[j] != '\0'){
				//printf("%c %d %c %d\n", s[i], i, s[j], j);
				//printf("%s\n\n", s);
				s[i] = s[j];
				i++, j++;
			}
		}
		else if(s[j] == ' '){
			s[i] = s[j];
			i++, j++;
			while(s[j] == ' '){ j++; }
		}
		else{
			s[i] = s[j];
			i++, j++;
		}
	}
	s[i] = '\0';

}

void separa_comandos(char p1[], char op[], char p2[]){

	int i, tamanho = strlen(p2), id_op;
	strcpy(p1, p2);
	// inicializar / apagar operação anterior
	op[0] = '\0';

	for(i = 0; i < tamanho; i++){ // procurar "|" ou "||" ou "&" ou "&&"

		if(p2[i] == '|' || p2[i] == '&' || p2[i] == ';'){
			// op | ou &
			if(p2[i - 1] == ' ' && p2[i + 1] == ' '){ // | ou & estará entre espaços
				// gravar qual op
				if(p2[i] == '|'){
					op[0] = '|';
				}
				else if(p2[i] == ';'){
					op[0] = ';';
				}
				else if(p2[i] == '&'){
					op[0] = '&';
				}
				op[1] = '\0';
				p2[tamanho] = '\0';
				// limitar o primeiro comando em p1
				p1[i] = '\0';
				// o segundo comando começa depois do op
				strcpy(p2, p2 + i + 2);
				// com o op |, precisamos do segundo comando, deve ser lido se nao foi fornecido
				if((int)strlen(p2) == 0 && op[0] == '|'){
					ler_teclado(p2);
				}
				break;
			}
			else if(p2[i - 1] == ' ' && p2[i] == p2[i + 1] && p2[i + 2] == ' '){
				// op && ou ||
				if(p2[i] == '&'){ op[0] = op[1] = '&'; }
				else if(p2[i] == '|'){ op[0] = op[1] = '|'; }
				
				op[2] = '\0';
				// limitando o primeiro comando
				p1[i] = '\0';
				// o segundo comando começa depois do op
				strcpy(p2, p2 + i + 3);
				// com o op && ou ||, precisamos do segundo comando, deve ser lido se nao foi fornecido
				if((int)strlen(p2) == 0){
					ler_teclado(p2);
				}
				break;
			}
		}

	}
	// se nao tiver op, temos somente um comando
	if((int)strlen(op) == 0){
		p2[0] = '\0';
	}

}

char* get_path(char comando[]){
	int i = 0;
	static char path[100];
	strcpy(path, "/bin/");
	// concatenar primeira palavra do comando
	while(comando[i] != ' ' && comando[i] != '\0'){
		path[i + 5] = comando[i];
		i++;
	}
	path[i + 5] = '\0';
	return path;
}

char** get_args(char comando[]){
	static char* args[100];
	char args_temp[100][100];
	int qnt_args = 0, i, j;
	i = 0; // iterador do comando
	j = 0; // iterador dos argumentos
	while(comando[i] != '\0'){
		if(comando[i] == ' '){
			//printf("teste -- %s\n", args[qnt_args]);
			args_temp[qnt_args][j] = '\0';
			args[qnt_args] = args_temp[qnt_args];
			qnt_args++;
			i++;
			j = 0;
		}
		else{
			args_temp[qnt_args][j] = comando[i];
			i++, j++;
		}
	}
	args[qnt_args] = NULL;
	//printf(" -- %d\n", qnt_args);
	return args;
}

void executa_Pipe(char *c1, char *c2){
//executar o primeiro comando pegar o resultado
//jogar o resultado do primeiro comando na entrada do segundo
	int fd1[2];
	int status, pid;
	pipe(fd1); //comunica que existe comandos

	pid = fork(); //cria um filho

	if(pid == 0){ //filho 1 que executa ls-l
    	close(fd1[0]); //fecha o extremo de leitura que não está sendo usado
    	dup2(fd1[1], STDOUT_FILENO); //copia a saida para o descritor
        close(fd1[1]); //fecha o extremo de escrita
		execv(get_path(c1), get_args(c1));
	}else{ //pai
		close(fd1[1]); //fecha o extremo de escrita que não está sendo usado
		pid = fork(); //cria um filho
		if(pid == 0){ //filho 2 que executa wc
			dup2(fd1[0], STDIN_FILENO);
			close(fd1[0]); //fecha o extremo de leitura que não está sendo usado
			execvp(get_path(c2), get_args(c2));
    	}
    	else{ //pai
			close(fd1[0]); //fecha o extremo de leitura que não está sendo usado
		}
	}

	
    //um wait para cada filho
	wait(&status); //filho 1
	wait(&status); //filho 2
}

int executa_comando(char c[]){
	int fd[2];
	pipe(fd);
	pid_t pid = fork();
	if(pid == 0){
		write(fd[1], "v", 1); // sucesso
		int result = execv(get_path(c), get_args(c));
		if(result == -1){
			char descartar[2];
			read(fd[0], descartar, 1);
			write(fd[1], "f", 1); // falha ao executar o comando			
		}
		exit(0);
	}
	else{
		int status;
		char filho[2];
		waitpid(pid, &status, 0);
		read(fd[0], filho, 1);
		//printf(" --- %s\n", filho);
		if(filho[0] == 'f'){ return 0; } // em caso de falha, retornar 0
		else{ return 1; } // sucesso retornar 1
	}
}

int exec_op_independente(char c1[], char c2[]){
	int result1, result2;

	result1 = executa_comando(c1);
	result2 = executa_comando(c2);
	if(result1 == 1 && result2 == 1){ return 0; } // retornar 0 para sucesso nos dois comandos
	else if(result1 == 0 && result2 == 1){ return 2; } // retornar 2 para sucesso no segundo comando
	else if(result1 == 1 && result2 == 0){ return 1; } // retornar 1 para sucesso no primeiro comando
	else if(result1 == 0 && result2 == 0){ return -1; } // -1 para falha nos dois comandos

}

int exec_op_ou(char c1[], char c2[]){
	int result;
	result = executa_comando(c1);
	//printf("dps ping %d\n", result);
	if(result == 1){ return 1; }
	else if(result == 0){
		//printf("echo\n");
		result = executa_comando(c2);
		if(result == 1){ return 2; }
		else{ return -1; }
	}
}

int main(){	
	char entrada[1001];
	char comandos[100][1001];
	char operador[3];
	char c1[1001], c2[1001];
	int qnt_comandos=0;
	char ops[100][3];
	int qnt_ops = 0;
	int i, result_exec, j=0;
	ler_teclado(entrada);
	qnt_comandos = qnt_ops = 0;
	do{
		separa_comandos(comandos[qnt_comandos], ops[qnt_ops], entrada);
		qnt_comandos++;
		qnt_ops++;
	} while((int)strlen(entrada) != 0);
	
	for(i=0; i<qnt_ops; i++){
		strcpy(operador, ops[i]);

		if(operador[0]=='|' && operador[1]=='|'){

			result_exec = exec_op_ou(comandos[i], comandos[i+1]);

			if(result_exec == -1){ i++; } // nao executou nenhum comando
			else if(result_exec == 1 || result_exec == 2){ // sucesso no primeiro ou no segundo comando
				while(ops[i][0] == '|' && ops[i][0] == '|'){ i++; } // pular todos os comandos que estejam entre ||
				i++;
			}


		}else if(operador[0]=='|' && operador[1]=='\0'){
			strcpy(c1, comandos[i]);
			i++;
			strcpy(c2, comandos[i]);
			executa_Pipe(c1, c2);
		}else if(ops[i][0]=='&' && ops[i][1]=='&'){
			if(executa_comando(comandos[i])==1){
				executa_comando(comandos[i+1]);
				i++;
			}
			else { i++; }
		}else if(operador[0]=='&' && operador[1]=='\0'){
			executa_comando(comandos[i]);
			long int processo = getpid();
			//printd("PID = %ld\n", processo);
			execlp("/bin/bg", "bg", processo, NULL);
		}else if(ops[i][0] == ';'){
			// result_exec retornará -1 quando nenhum comando for executado, 0 se os dois comandos foram executados, 1 se só o primeiro comando foi executado e 2 se só o segundo
			result_exec = exec_op_independente(comandos[i], comandos[i+1]);
			i++;
		}else{ // sem operação, somente um comando
			executa_comando(comandos[i]);
		}
	}
	return 0;
}
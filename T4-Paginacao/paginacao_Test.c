/*
    Projeto 04: Algoritmos de Paginacao
    *--------*-----------------------*
    |   RA   | Aluno                 |
    *--------*-----------------------*
    | 143196 | Bruno Rasera          |
    | 140620 | Daniel de Souza Paiva |
    | 139969 | Letícia A. P. Lisboa  |
    | 140886 | Mateus Gomes Ferreira |
    *--------------------------------*
*/

/* RASCUNHOS -> LEMBRAR DE APAGAR O QUE NAO FOR ENVIAR!!:
 * - Quanto menos um algoritmo gerar pageMiss, mais evitado será o swap. 
 * - Um critério para determinar se uma solução é melhor que outra é o numero de pagemiss
 * - Primeiramente os campos da memoria real devem ser preenchidas:
 * - Preenche com páginas, assuma que a memoria real (mr) tá cheia.
 * - Dado um endereço de memoria vcs precisam percorrer a estrutura da memoria virtual e verificar se aquele endereço está ausente ou presente
 *
 *  Como podemos organizar a memoria virtual?
 *  - Se as paginas tem deslocamento de 12bits, tenho então um espaço de 0 a 4k, 
 *  - Se eu estou fazendo uso de deslocamento de 12bits, presume-se que serão usados valores de 0 a 4096. 
 *  - Qual a página responsável pelo endereço 3000? A memória virtual para esse exemplo está sendo organizada de modo sequencial, na hora de acessar
 *  a pagina, verifica-se se ela está presente ou ausente. Se acessou a página e a página está presente, então não fez nada, só atualiza a contagem de
 *  conferências feitas mas nn mexe na de pagemiss. (LEMBRAR DE ATUALIZAR O BIT REFERENCIADA!!)
 *  - Posso gerar eventos de memoria onde ocorrem mudanças na página (alterar o bit modificada) 
 * 
 * //garantir mesmos inputs pelo menos 3 vezes, 
 * X -> Configurações em numeros de processos ou faixa de aleatoriedades (tamanho de processos);
 * (A quantidade de processos pode ser pequena mas um pode ter 10 pag, outro 20, outro 30)
 * 
 * Conjunto de processos onde o tamanho das paginas variam
 * Ver as possibilidades de X processos de tamanho 10, Y processos de tamanho 20, Z processos de tamanho 30
 * 
 * Ver tbm a possibilidade de processos de mesmo tamanho
 * 
 * No relatório é importante aprimorar a analise critica sobre as coisas, é importante não so narrar aquele resultado, mas
 * tambem discutir aquilo, refletir sobre o funcionamento do seu algoritmo e pq q esse funcionamento refletiu esse valor
 * 
 * 2 casos de teste: (3 exec no min pra cada, em cada algoritmo)
 * 1- Tam fixo pra memoria, variando a qtd de paginas que uma quatidade de processos tem acesso, ex: x processos c 10
 * x processos com tamanho 10, y processos com tamanho 20 e z processos de tamanho 30, sendo NProx = x+y+z
 * 
 * 2- processos de mesmo tamanho
 * 
 * Quantificar a qtd de page miss e de acesos a memoria (lembrar que guardar a pag conta um acesso e carregar tbm conta um)
 */

/* ################### BIBLIOTECAS.H #################### */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

/* ############# CONFIGURACOES DA SIMULACAO ############# */

/* Escolha do algoritmo de paginacao utilizado
 * 1 -> NUR
 * 2 -> RELOGIO
*/
# define algoritmo 1

/* Escolha do tipo da simulacao
 * 1 -> Simulacao onde blocos de processos possuem qtd diferentes de paginas de memorias em cada bloco
 * 0 -> Simulacao onde todos os processos possui acesso a mesma qtd de paginas de memorias
*/
#define heterogenea 1

/* Escolha do caso a ser simulado
 * 1, 2 ou 3 -> Aplicavel em ambas as simulacoes
*/
#define case 3

/* ############# DEFINICAO DAS CONSTANTES ############# */
#if case == 1
    const int SMV = 200;
    const int N_PROC = 20;
#elif case == 2
    const int SMV = 400;
    const int N_PROC = 50;
#elif case == 3
    const int SMV = 800;
    const int N_PROC = 100;
#endif

#define N_ACCESS 100
const int SMR = 50;


/*#define pags_p1 10
#define pags_p2 20
#define pags_p3 30

#define p1 30
#define p2 20
#define p3 10*/


//const int N_PROC = 100; //Definido nos ifs


//const int SMV = 800; //Definido nos ifs

//const int SSW = SMV - SMR;
//MV > MR e SW = MV - MR

//Contagens
int pageMisses = 0;
int memAccesses = 0;

char dirPages[100];//GLOBAL!!

int presenteTotal = 0;

/* ############# <pagina_t.h> ############# */
typedef struct pagina_t
{
    uint16_t presente:1;//bitsignal->usando apenas 1bit dos 16alocados pelo uint16_t
    uint16_t referenciada:1;
    uint16_t modificada:1;
    uint16_t pad:1;//Pra fechar os 16bits
    uint16_t deslocamento:12;//12bits da 4kb e o primeiro pagina t seria de 0 a 4kb na memoria real, ele é o mapeamento da mem vitual pra real
    //se smv é 2 MV[0]->Ok a 4k MV[1]=4k a 8k, o deslocamento do primeiro seria a representação de 0kb a 4k
    //Ele é só uma representação... Se a CPU quer acessar 5300, vc só teria que ver onde que isso da na memoria virtual, o primeiro é de 0 a 4 e o outro de 5 a 8, então vou no segundo
    
} *PaginaT;

PaginaT mv;

/* ############# <processo_t.h> ############# */
typedef struct processo_t
{
    int first;
    int last;

} *ProcessoT;

ProcessoT proc;

void init()
{
    int p1 = 0.25 * SMV / (0.5 * N_PROC);
    int p2 = 0.25 * SMV / (0.25 * N_PROC);
    int p3 = 0.5 * SMV / (0.25 * N_PROC);
    mv = (PaginaT) calloc(SMV, sizeof(struct pagina_t));
    proc = (ProcessoT) calloc(N_PROC, sizeof(struct processo_t));
    int aux = 0;
    int fdAux; //ponteiro para a pagina atual
    char pageNum[100], charAux[1];
    int i;
    
    //BLOCO DE CRIACAO DAS PAGINAS DE MEMORIA E SEU DIRETORIO EM DISCO:
    fdAux = open("count.txt", O_RDONLY, S_IRUSR | S_IWUSR);
    if (fdAux == -1) { //erro de abertura
		perror("open: ");
		exit(1);
	}

    read(fdAux, charAux, 1);//Ler a contagem atual
    close(fdAux);//Resetar o ponteiro do arquivo
    charAux[0]++;//incrementando o contador de teste

    fdAux = open("count.txt", O_WRONLY, S_IRUSR | S_IWUSR);
    if (fdAux == -1) { //erro de abertura
		perror("open: ");
		exit(1);
	}

    write(fdAux, charAux, 1);//atualizando o contador no arquivo
    close(fdAux);//fechando o arquivo
    sprintf(dirPages,"memPages_test%d",charAux[0]);//criando o nome do diretorio
    mkdir(dirPages);//Criando o diretorio dos testes
    
    //Definindo as paginas de cada processo
    if(heterogenea){
        for(i = 0; i < N_PROC; i++)
        {
            proc[i].first = aux;

            if (i < N_PROC * 0.5) proc[i].last = proc[i].first + p1 - 1;
            else if (i < N_PROC * 0.75) proc[i].last = proc[i].first + p2 - 1;
            else proc[i].last = proc[i].first + p3 - 1;

            aux = proc[i].last + 1;
        }
    } else{
        for(i = 0; i < N_PROC; i++)
        {
            proc[i].first = aux;

            proc[i].last = proc[i].first + (SMV/N_PROC) - 1;

            aux = proc[i].last + 1;
        }
    }
    
    //Inicializando as paginas de memoria
    for(i = 0; i < SMV; i++)
    {
        //mv[i].deslocamento
        mv[i].pad = 0;
        /* Memoria real com espacos livres */
        if (presenteTotal < SMR) mv[i].presente = rand()%2;
        /* Memoria real sem espacos livres */
        else mv[i].presente = 0;
        /* Se a pagina estiver em memoria, aquela pagina pode ser 
         * referenciada ou modificada
         */
        if (mv[i].presente) 
        {
            presenteTotal++;
            //Precisamos atribuir isso para a primeira iteracao do algoritimo NUR
            mv[i].referenciada = rand()%2;
            mv[i].modificada = rand()%2;
        }
        /* Caso contrario, nao*/
        else
        {
            mv[i].referenciada = 0;
            mv[i].modificada = 0;
        }
        
        //Pagina ok -> carregar em memoria
        
        sprintf(pageNum, "%s/%d.txt",dirPages,i);
        fdAux = open(pageNum, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);//criando a pagina no diretorio de paginas daquele teste
        if (fdAux == -1) { //erro de abertura
		    perror("open: ");
		    exit(1);
	    }
        close(fdAux);
    }
}

/* void zeroReferences(void):
 * Procedimento utilitario que zera o bit referenciada de todas as paginas de memoria
 */
void zeroReferences()
{
    for(int i = 0; i < SMV; i++)
        if (mv[i].referenciada) mv[i].referenciada = 0;
}

/* void acessMem(int save, int pageNum):
 * Args:
 * -> save: 1 -> Salvar a pagina em memoria; 0 -> Carregar a pagina
 * -> pageNum: numero da pagina acessada
 * 
 * Procedimento que simula um acesso a memoria para salva-la ou carrega-la
 */
void acessMem(int save, int pageNum){
    int pageFile;
    char pageAddr[100];
    sprintf(pageAddr, "%s/%d.txt",dirPages,pageNum);
    if (save) {
        pageFile = open(pageAddr, O_WRONLY, S_IRUSR | S_IWUSR);
        if (pageFile == -1) { //erro de abertura
		    perror("open: ");
		    exit(1);
	    }
    } else {
        pageFile = open(pageAddr, O_RDONLY, S_IRUSR | S_IWUSR);
        if (pageFile == -1) { //erro de abertura
		    perror("open: ");
		    exit(1);
	    }
    }
    
    close(pageFile);
}
/* ############# ALGORITMO NUR ############# */

void nur()
{
    int class1 = -1, class2 = -1, class3 = -1;

    for (int i = 0; i < SMV; i++)
    {   
        if(mv[i].presente)
        {
            //Classe 0: nao referenciada e nao modificada
            if (!mv[i].referenciada && !mv[i].modificada)
            {
                memAccesses++;
                mv[i].presente = 0;
                return;
            }
            //Classe 1: nao referenciada e modificada
            else if (!mv[i].referenciada && mv[i].modificada) class1 = i;

            //Classe 2: referenciada e nao modificada
            else if (mv[i].referenciada && !mv[i].modificada)  class2 = i;

            //Classe 3: referenciada e modificada
            else class3 = i;
        }
    }
    //Caso nao seja encontrado NENHUM da classe 0, removo o ultimo encontrado da classe mais proxima da 0...
    if (class1 >= 0)
    {//Preciso de dois acessos, um para salvar a pagina atual e outro para carregar a nova
        acessMem(1,class1);//Salvando o conteudo da pagina descarregada em memoria
        memAccesses += 2; //note que o outro acesso eh efeitivamente simulado no laco da main
        mv[class1].presente = 0;
        mv[class1].modificada = 0;
    }
    else if (class2 >= 0)
    {//somente carrego a nova, a atual nao precisa ser salva pois nao foi modificada
        memAccesses++; //note que esse acesso eh efeitivamente simulado no laco da main
        mv[class2].presente = 0;
        mv[class2].referenciada = 0;
    }
    else if (class3 >= 0)
    {//preciso carregar a nova e salvar a atual em memoria
        acessMem(1,class3);//Salvando o conteudo da pagina descarregada em memoria
        memAccesses += 2; //note que o outro acesso eh efeitivamente simulado no laco da main
        mv[class3].presente = 0;
        mv[class3].referenciada = 0;
        mv[class3].modificada = 0;
    }
}

/* ############# ALGORITMO RELOGIO ############# */

int ponteiroRelogio = 0;

void relogio()
{
    while(1)
    {
        if (mv[ponteiroRelogio].presente)
        {
            if (!mv[ponteiroRelogio].referenciada)
            {//Nao referenciada, removo essa primeiro
                mv[ponteiroRelogio].presente = 0;
                memAccesses++; //Esse acesso efetivamente sera simulado no laco da main
                if (mv[ponteiroRelogio].modificada) 
                {//Se esta modificada, preciso salvar ela em memoria e atualizar que agora ela esta com a versao mais recente salva
                    mv[ponteiroRelogio].modificada = 0; //logo, nao mais modificada
                    acessMem(1,ponteiroRelogio);//Salvando o conteudo da pagina descarregada em memoria
                    memAccesses++;
                }
                return;
            }
            else mv[ponteiroRelogio].referenciada = 0;
        }

        ponteiroRelogio = (ponteiroRelogio + 1) % SMV;
    }
}

/* ############# PROGRAMA PRINCIPAL ############# */

int main(int argc, char **argv)
{
    time_t t;
    srand((unsigned) time(&t));

    init();

    for(int i = 0; i < N_PROC; i++) printf("proc %d: %d %d\n", i, proc[i].first, proc[i].last);

    for(int i = 0; i < SMV; i++) printf("pag %d: m %d; p %d; r %d\n", i, mv[i].modificada, mv[i].presente, mv[i].referenciada);

    for(int i = 0; i < N_ACCESS; i++) //seja i o intervalo de tempo atual
    {
        for(int j = 0; j < N_PROC; j++)
        {
            /* Gerando um acesso a uma pagina aleatoria 
             * dentro dos limites daquele processo 
             */
            int upper = proc[j].last;
            int lower = proc[j].first;
            int tryingAccess = (rand() % (upper - lower + 1)) + lower;

            /* Se nao esta presente: Page Miss */
            if (!mv[tryingAccess].presente)
            { //Chamando o algoritmo de paginacao:
                if (algoritmo == 1) nur();
                else if (algoritmo == 2) relogio();
                pageMisses++;
                acessMem(0,tryingAccess);//Carregando a pagina faltante
            }

            /* Note que se a pagina atual nao estava presente, 
             * o algoritmo de paginacao ja liberou espaco para que
             * a pagina necessaria fosse carregada, entao atualizamos 
             * o bit presente para 1 SOMENTE depois de garantir que,
             * caso ausente, ja foi liberado espaco pelo paginador
            */
            mv[tryingAccess].presente = 1;

            /* Se esta presente: Pode ser modificada e foi referenciada */
            mv[tryingAccess].modificada = rand()%2;
            mv[tryingAccess].referenciada = 1;
            
        }

        /* Apos todos os processos acessarem suas paginas
         * as referencias sao zeradas (passou 1 intervalo de tempo)
         */
        zeroReferences();
    }

    printf("Total page misses: %d\n", pageMisses);
    printf("Total memory accesses: %d\n", memAccesses);

    char resultsDir[100], resultsText[200];

    sprintf(resultsDir, "%s/_results.txt",dirPages);
    int fdRes = open(resultsDir, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fdRes == -1) { //erro de abertura
		    perror("open: ");
		    exit(1);
	    }
    int n = sprintf(resultsText, "Total page misses: %d\nTotal memory accesses: %d\n", pageMisses, memAccesses);
    write(fdRes, resultsText, n);
    close(fdRes);
}
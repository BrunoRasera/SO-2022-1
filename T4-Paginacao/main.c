#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define SMV 100
#define SMR 50
const int SSW = SMV - SMR;
//MV > MR e SW = MV - MR

#define N_PROC 10
#define N_ACCESS 100

int pageMisses = 0;

int presenteTotal = 0;

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

typedef struct processo_t
{
    int first;
    int last;

} *ProcessoT;

ProcessoT proc;

//Quanto menos um algoritmo gerar pageMiss, mais evitado será o swap. 
//Um critério para determinar se uma solução é melhor que outra é o numero de pagemiss
//Primeiramente os campos da memoria real devem ser preenchidas:
/* Preenche com páginas, assuma que a memoria real (mr) tá cheia.
 * Dado um endereço de memoria vcs precisam percorrer a estrutura da memoria virtual e verificar se aquele endereço está ausente ou presente

 Como podemos organizar a memoria virtual?
 - Se as paginas tem deslocamento de 12bits, tenho então um espaço de 0 a 4k, 
 - Se eu estou fazendo uso de deslocamento de 12bits, presume-se que serão usados valores de 0 a 4096. 
 - Qual a página responsável pelo endereço 3000? A memória virtual para esse exemplo está sendo organizada de modo sequencial, na hora de acessar
 a pagina, verifica-se se ela está presente ou ausente. Se acessou a página e a página está presente, então não fez nada, só atualiza a contagem de
 conferências feitas mas nn mexe na de pagemiss. (LEMBRAR DE ATUALIZAR O BIT REFERENCIADA!!)
 - Posso gerar eventos de memoria onde ocorrem mudanças na página (alterar o bit modificada) 
 */

void init()
{
    mv = (PaginaT) calloc(SMV, sizeof(struct pagina_t));
    proc = (ProcessoT) calloc(N_PROC, sizeof(struct processo_t));
    int aux = 0;
    for(int i = 0; i < N_PROC; i++)
    {
        proc[i].first = aux;
        //proc[i].last = proc[i].first + 2 + rand()%4;
        proc[i].last = proc[i].first + SMV/N_PROC - 1 + 2 - rand()%5;
        if (proc[i].last >= SMV) proc[i].last = SMV - 1;
        aux = proc[i].last + 1;
    }

    proc[N_PROC-1].last = SMV - 1;

    for(int i = 0; i < SMV; i++)
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
            mv[i].referenciada = rand()%2;
            mv[i].modificada = rand()%2;
        }
        /* Caso contrario, nao*/
        else
        {
            mv[i].referenciada = 0;
            mv[i].modificada = 0;
        }
    }
}

void zeroReferences()
{
    for(int i = 0; i < SMV; i++)
        if (mv[i].referenciada) mv[i].referenciada = 0;
}

int main()
{
    time_t t;
    srand((unsigned) time(&t));

    init();

    for(int i = 0; i < N_PROC; i++) printf("proc %d: %d %d\n", i, proc[i].first, proc[i].last);

    for(int i = 0; i < SMV; i++) printf("pag %d: m %d; p %d; r %d\n", i, mv[i].modificada, mv[i].presente, mv[i].referenciada);

    for(int i = 0; i < N_ACCESS; i++)
    {
        for(int j = 0; j < N_PROC; j++)
        {
            /* Gerando um acesso a uma pagina aleatoria 
             * dentro dos limites daquele processo 
             */
            int upper = proc[i].last;
            int lower = proc[i].first;
            int access = (rand() % (upper - lower + 1)) + lower;

            /* Page Miss */
            if (!mv[access].presente)
            {
                //chamar escalonador aqui
                pageMisses++;
            }
            /* Pode ser modificada e foi referenciada */
            mv[access].modificada = rand()%2;
            mv[access].referenciada = 1;
        }

        /* Apos todos os processos acessarem suas paginas
         * as referencias sao zeradas
         */
        zeroReferences();
    }

    printf("Total page misses: %d", pageMisses);
}

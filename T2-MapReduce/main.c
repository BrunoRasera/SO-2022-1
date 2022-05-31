#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

FILE *source;

/* SIZE is the size of the hash table */
#define SIZEHASH 211

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* Lista para cada posição populada da tabela */
typedef struct BucketListRec
{
    char *valor;
    int num;
    struct BucketListRec *next;
} * BucketList;

/* Declaração da Hash Table */
BucketList hashTable[SIZEHASH];

#define MAXLINESIZE 1000
#define MAXLINENUMBER 10

sem_t mutex[SIZEHASH];
int vazio[SIZEHASH];
int mappersWorking;
int reducersWorking[SIZEHASH];
int itens[SIZEHASH];

/* Procedimento que devolve a posição
 * na tabela hash para uma dada key
 */
int hash(char *key)
{
    int temp = 0;
    int i = 0;
    while (key[i] != '\0')
    {
        temp = ((temp << SHIFT) + key[i]) % SIZEHASH;
        ++i;
    }
    return temp;
}

/* Procedimento que insere uma
 * palavra na tabela Hash
 */
void insereHash(char *palavra)
{
    int h = hash(palavra);
    sem_wait(&mutex[h]);
    BucketList l = hashTable[h], ant = NULL;
    while (l != NULL)
    {
        ant = l;
        l = l->next;
    }
    l = (BucketList)malloc(sizeof(struct BucketListRec));
    l->valor = palavra;
    l->num = 1;
    l->next = NULL;
    if (ant == NULL) hashTable[h] = l;
    else ant->next = l;
    sem_post(&mutex[h]);
    if (vazio[h]) vazio[h] = 0;
    itens[h] += 1;
}

void *mapper(void *args)
{
    char *str = (char *)args;
    char delimiters[10] = "- .,\n\0";
    char *token;

    token = strtok(str, delimiters);

    while (token != NULL)
    {
        //printf("%s ", token);
        insereHash(token);
        token = strtok(NULL, delimiters);
    }
    //printf("terminou mapper\n");

    mappersWorking--;
    
    pthread_exit(NULL);
}

int buscaRepete(int h, BucketList l)
{
    BucketList ant = l,  atual = l->next;
    int flag = 0;

    while(atual != NULL){
        if (strcmp(atual->valor,l->valor) == 0)
        {
            sem_wait(&mutex[h]);
            ant->next = atual->next;
            l->num = l->num + atual->num;
            free(atual);
            atual = ant;
            if (!flag) flag = 1;
            itens[h] -= 1;
            sem_post(&mutex[h]);
        }
        ant = atual;
        atual = atual->next;
    }
    if (flag) return 1;
    else return 0;
}

void *reducer(void *args) {
    int h = *(int *)args;
    int cont = 0;
    BucketList l = hashTable[h];

    //printf("%d ", h);

    while (1)
    {
        if (!vazio[h])
        {
            if(buscaRepete(h, l)) cont = 0;
        }
        if(!mappersWorking && cont == itens[h])
        {
            reducersWorking[h] = 0;
            pthread_exit(NULL);
        }
        if(l->next == NULL) l = hashTable[h];
        else l = l->next;

        cont++;
    }

    pthread_exit(NULL);
}

void init(void) {
	for (int i = 0; i < SIZEHASH; i++) {
        sem_init(&mutex[i], 0, 1);
        vazio[i] = 1;
        reducersWorking[i] = 1;
        itens[i] = 0;
    }
}

struct thread_t {
	pthread_t t;
	int h;
};

typedef struct thread_t thread_t;

int main(int argc, char *argv[])
{
    char pgm[120]; /* source code file name */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    strcpy(pgm, argv[1]);
    if (strchr(pgm, '.') == NULL)
        strcat(pgm, ".txt");
    source = fopen(pgm, "r");
    if (source == NULL)
    {
        fprintf(stderr, "File %s not found\n", pgm);
        exit(1);
    }

    char palavras[MAXLINENUMBER][MAXLINESIZE];
    int linhas = 0;

    init();

    pthread_t map[MAXLINESIZE];

    thread_t red[SIZEHASH];

    while (fgets(palavras[linhas], MAXLINESIZE, source) != NULL)
    {
        palavras[linhas][strcspn(palavras[linhas], "\r\n")] = 0;
        //printf("%s", palavras[linhas]);
        pthread_create(&map[linhas], NULL, mapper, palavras[linhas]);
        linhas++;
        mappersWorking++;
    }

    for(int i = 0; i < SIZEHASH; i++)
    {
        pthread_create(&red[i].t, NULL, reducer, &red[i].h);
        red[i].h = i;
    }

    for(int i = 0; i < linhas; i++)
    {
        pthread_join(map[i], NULL);
    }

    for(int i = 0; i < SIZEHASH; i++)
    {
        pthread_join(red[i].t, NULL);
    }

    for(int i = 0; i < SIZEHASH; i++)
    {
        BucketList atual = hashTable[i];
        while(atual != NULL)
        {
            printf("%s %d\n", atual->valor, atual->num);
            atual = atual->next;
        }
    }

    /*for(int i = 0; i < SIZEHASH; i++)
    {
        printf("%d ", vazio[i]);
    }*/

    //printf("%d", mappersWorking);

    fclose(source);
    return 0;
}
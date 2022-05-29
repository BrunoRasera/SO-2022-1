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
#define MAXLINENUMBER 1000

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
    char delimiters[5] = "-\n ";
    char *token;

    token = strtok(str, delimiters);

    while (token != NULL)
    {
        insereHash(token);
        token = strtok(NULL, delimiters);
    }

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
            l->num = l->num + 1;
            free(atual);
            atual = ant;
            flag = 1;
            itens[h] -= 1;
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
        l = l->next;
    }
}

void init(void) {
	for (int i = 0; i < SIZEHASH; i++) {
        sem_init(&mutex[i], 0, 1);
        vazio[i] = 1;
        reducersWorking[i] = 1;
        itens[i] = 0;
    }
}

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

    pthread_t map[MAXLINESIZE], red[SIZEHASH];

    while (fgets(palavras[linhas], MAXLINESIZE, source) != NULL)
    {
        pthread_create(&map[linhas], NULL, mapper, palavras);
        linhas++;
        mappersWorking++;
    }

    for(int i = 0; i < SIZEHASH; i++)
    {
        pthread_create(&red[i], NULL, reducer, &i);
    }

    for(int i = 0; i < linhas; i++)
    {
        pthread_join(map[i], NULL);
    }
    for(int i = 0; i < SIZEHASH; i++)
    {
        pthread_join(red[i], NULL);
    }

    for(int i = 0; i < SIZEHASH; i++)
    {
        BucketList atual = hashTable[i];
        if (atual != NULL)
        {
            while(atual != NULL)
            {
                printf("%s %d\n", atual->valor, atual->num);
            }
        }
    }

    fclose(source);
    return 0;
}

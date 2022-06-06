/*
    Projeto 02: Map Reduce
    *--------*-----------------------*
    |   RA   | Aluno                 |
    *--------*-----------------------*
    | 143196 | Bruno Rasera          |
    | 140620 | Daniel de Souza Paiva |
    | 139969 | Letícia A. P. Lisboa  |
    | 140886 | Mateus Gomes Ferreira |
    *--------------------------------*

    Usage: 
    -> Compiled as: gcc -Wall -o mapReduce ./mapred.c
    -> mapReduce *.txt

    Function:
    -> Given a *.txt file, creates a *_out.txt file that contains the number of ocurrences of 
       each word in the given text file in the format: <word> <count>
*/

/*##############< Libs Include >#############*/
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

/*##############< Size Parameters >#############*/
#define MAXLINESIZE 1000
#define MAXLINENUMBER 1000

/*##############< Hash Table Definition >#############*/

/* SIZE is the size of the hash table */
#define SIZEHASH 211

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* List to each position of the Hash Table
 * - Used for deal with collisions
 */
typedef struct BucketListRec
{
    char *valor; //word
    int num; //amount
    struct BucketListRec *next; //pointer to next position
} * BucketList;


/* int hash(char *key)
 * -> Given a "key" string, calculate and return the 
 *    position that it will be inserted into 
 *    the hash table
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

/* HashTabele declaration */
BucketList hashTable[SIZEHASH];

/*##############< Semafors and GLobal Control Vars >#############*/
sem_t mutex[SIZEHASH]; //Mutex for each critical region(tails) of the HashTabel
int vazio[SIZEHASH]; //Controls the reducers work, avoids reducing a empty position
int mappersWorking; //Counter of the amount of mappers working, if no mapper is working, reducers can stop
int itens[SIZEHASH]; //Counter to the amount of items into each bucket list

/* void init(void)
 * -> Initialize the pre declarated semafors
 */
void init(void) 
{
	for (int i = 0; i < SIZEHASH; i++) {
        sem_init(&mutex[i], 0, 1);
        vazio[i] = 1;
        itens[i] = 0;
    }
}

struct thread_t 
{
	pthread_t t;
	int h;
};

typedef struct thread_t thread_t;

/*##############< Mapper >#############*/

/* void insereHash(char *palavra)
 * -> Given a string "palavra", inserts it in the
 *    hash table as 1 ocurrence
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

/* void mapper(void *args)
 * -> Program that given a string as an "args"
 *    breaks it into words and insert them 
 *    into the hash table as 1 ocurrence
 * Exemple: If it reads the word "test" it will put it in
 * the hash table with 1 ocurrence
 */
void *mapper(void *args)
{
    char *str = (char *)args;
    char *delimiters = "- .,;:\"\'";
    char *token;

    token = strtok_r(str, delimiters, &str);

    while (token != NULL)
    {
        insereHash(token);
        token = strtok_r(str, delimiters, &str);
    }
    
    mappersWorking--;
    
    pthread_exit(NULL);
}

/*##############< Reducer >#############*/

/* int buscaRepete(int h, BucketList l)
 * -> Program that given a "h" hash table position
 *    and the "l" Bucket List of that position,
 *    iterates it searching and remving duplicated itens
 *    * If theres any duplicated item, merges them
 *      adding both numbers of ocurrence
 * -> Return 0 if no duplicated items was found
 */
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

/* void reducer(void *args)
 * -> Program that given a hash table position as "args"
 *    iterates continuosly over the bucket list in that position
 *    calling "buscaRepete" funcion to join the duplicated items
 *    throwed by the mappers, Stops when theres no mapper working and 
 *    theres no duplicated items in that bucket list.
 */
void *reducer(void *args) 
{
    int h = *(int *)args;
    int cont = 0;
    BucketList l = hashTable[h];

    while (1)
    {
        if (!vazio[h] && l != NULL)
        {
            if(buscaRepete(h, l)) cont = 0;
        }
        if(!mappersWorking && cont == itens[h])
        {
            pthread_exit(NULL);
        }
        if(hashTable[h] == NULL || l->next == NULL) l = hashTable[h];
        else l = l->next;

        cont++;
    }

    pthread_exit(NULL);
}

/*##############< Main Program >#############*/
int main(int argc, char *argv[])
{
    char pgm[120]; // source code file name
    FILE *source; //Pointer to the source file
    FILE *out; //Pointer to output file

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
        fprintf(stderr, "Error opening source file: File %s not found\n", pgm);
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
        pthread_create(&map[linhas], NULL, mapper, palavras[linhas]);
        linhas++;
        mappersWorking++;
    }
    
    fclose(source); //Finished using the source file, so we close it sooner as possible
    
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
    
    //Adjusting output file name
    pgm[strlen(pgm)-4] = '\0';
    strcat(pgm,"_out.txt");
    //Only open the output file AFTER threads finished its jobs
    out = fopen(pgm,"w");
    if (out == NULL)
    {
        fprintf(stderr, "Error in writing output file: %s\n", pgm);
        exit(1);
    }
    //Printing the header of the output file
    fprintf(out,"*-------------------------------------\n| Output file for txt: %s\n| Standard: <WORD> <OCURRENCES>\n*-------------------------------------\n", argv[1]);
    
    //Printing the hash table into the output file
    for(int i = 0; i < SIZEHASH; i++)
    {
        BucketList atual = hashTable[i];
        while(atual != NULL)
        {
            fprintf(out,"%s %d\n", atual->valor, atual->num);
            atual = atual->next;
        }
    }
    
    fclose(out); //Closing output file, since we finished using it
    
    //Free the allocated memory for the hash table
    for(int i = 0; i < SIZEHASH; i++)
    {
        BucketList atual = hashTable[i];
        while(atual != NULL)
        {
            BucketList aux = atual->next;
            free(atual);
            atual = aux;
        }
    }

    
    return 0;
}

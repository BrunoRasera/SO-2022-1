#include <stdint.h>
#define SMV 1
#define SMR 2
#define SSW 3

struct pagina_t
{
    uint16_t presente:1;
    uint16_t referenciada:1;
    uint16_t modificada:1;
    uint16_t pad:1;
    uint16_t deslocamento:12;
};

struct pagina_t mv[SMV], mr[SMR], sw[SSW];

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
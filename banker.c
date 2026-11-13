/*
 * banker.c - Algoritmo do Banqueiro (Implementacao 6)
 *
 * Sessao 1: validacao dos argumentos de linha de comando.
 * Cada argumento depois do nome do programa e o total de instancias
 * de um recurso do sistema.
 *
 * Sessao 2: leitura do customer.txt e montagem da matriz maximum.
 *
 * Uso: ./banker <total_r0> <total_r1> ... <total_rN>
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQUIVO_CUSTOMER "customer.txt"

/* Limita o comprimento de UMA linha do arquivo, nao a quantidade de
   clientes nem a de recursos: essas duas continuam descobertas em tempo
   de execucao, como o enunciado exige. */
#define TAMANHO_MAXIMO_DA_LINHA 1024

/* As duas mensagens de erro previstas no enunciado.
   TODO: trocar o texto abaixo pelo texto literal do enunciado. */
#define ERRO_QUANTIDADE_DE_RECURSOS                                          \
    "Erro: a quantidade de recursos informada na linha de comando e "        \
    "diferente da quantidade de recursos do arquivo " ARQUIVO_CUSTOMER ".\n"
#define ERRO_FORMATO_DO_ARQUIVO                                              \
    "Erro: o arquivo " ARQUIVO_CUSTOMER " nao esta no formato indicado.\n"

/*
 * Converte texto em inteiro nao negativo.
 * Devolve 0 em caso de sucesso e grava o valor em *destino.
 * Devolve -1 se o texto nao for um inteiro nao negativo valido.
 *
 * Usa strtol em vez de atoi porque strtol informa, pelo ponteiro fim,
 * onde parou de converter: se fim nao aponta para o terminador da
 * string, o argumento tinha lixo ("10a", "abc", "") e deve ser recusado.
 * atoi devolveria 0 nesses casos, fingindo que entendeu o argumento.
 */
static int converter_inteiro_nao_negativo(const char *texto, int *destino) {
    char *fim = NULL;
    long valor;

    if (texto == NULL || *texto == '\0') {
        return -1;
    }

    errno = 0;
    valor = strtol(texto, &fim, 10);

    if (*fim != '\0') {          /* sobrou caractere nao numerico */
        return -1;
    }
    if (errno == ERANGE) {       /* estourou o long */
        return -1;
    }
    if (valor < 0 || valor > INT_MAX) {
        return -1;
    }

    *destino = (int) valor;
    return 0;
}

/*
 * Remove do fim da linha os caracteres '\n' e '\r' e devolve o novo
 * comprimento.
 *
 * Nao exigir o '\n' para considerar a linha valida e o que permite ler a
 * ultima linha do customer.txt do professor, que termina em "4,3,3" sem
 * quebra de linha. O '\r' importa porque arquivos de teste salvos por um
 * editor do Windows terminam em CRLF: sem essa limpeza o strtol
 * enxergaria lixo depois do ultimo numero de um arquivo correto.
 */
static int remover_fim_de_linha(char *linha) {
    int tamanho = (int) strlen(linha);

    while (tamanho > 0 &&
           (linha[tamanho - 1] == '\n' || linha[tamanho - 1] == '\r')) {
        linha[--tamanho] = '\0';
    }
    return tamanho;
}

/*
 * Devolve 1 se a linha nao tem nada alem de espacos em branco.
 * Essas linhas sao ignoradas em vez de recusadas: uma linha so com
 * espacos nao declara cliente nenhum, entao nao e um cliente com a
 * largura errada.
 */
static int linha_em_branco(const char *linha) {
    int i;

    for (i = 0; linha[i] != '\0'; i++) {
        if (!isspace((unsigned char) linha[i])) {
            return 0;
        }
    }
    return 1;
}

/*
 * Conta quantos valores a linha declara: um a mais que o numero de
 * virgulas. Olha so a forma da linha; validar o conteudo de cada campo
 * e tarefa de ler_valores_da_linha.
 */
static int contar_valores_na_linha(const char *linha) {
    int valores = 1;
    int i;

    for (i = 0; linha[i] != '\0'; i++) {
        if (linha[i] == ',') {
            valores++;
        }
    }
    return valores;
}

/*
 * Le quantidade_de_valores inteiros nao negativos separados por virgula
 * e grava em destino[0..quantidade_de_valores-1].
 * Devolve 0 em caso de sucesso e -1 se a linha nao estiver no formato.
 *
 * Exigir um digito no inicio de cada campo descarta campo vazio, "-3" e
 * "+3" de uma vez; conferir o ponteiro fim garante que o que veio depois
 * do numero e exatamente o separador esperado (virgula no meio da linha,
 * terminador no ultimo campo), o que recusa "7,,3", "7,5,3," e "7,5a,3".
 */
static int ler_valores_da_linha(const char *linha, int *destino,
                                int quantidade_de_valores) {
    const char *posicao = linha;
    int i;

    for (i = 0; i < quantidade_de_valores; i++) {
        char *fim = NULL;
        long valor;

        if (!isdigit((unsigned char) *posicao)) {
            return -1;           /* campo vazio, sinal ou caractere invalido */
        }

        errno = 0;
        valor = strtol(posicao, &fim, 10);

        if (errno == ERANGE || valor > INT_MAX) {
            return -1;           /* nao cabe em int */
        }

        if (i < quantidade_de_valores - 1) {
            if (*fim != ',') {   /* faltou a virgula ou veio lixo antes dela */
                return -1;
            }
            posicao = fim + 1;
        } else if (*fim != '\0') {
            return -1;           /* sobrou coisa depois do ultimo valor */
        }

        destino[i] = (int) valor;
    }
    return 0;
}

/*
 * Le o customer.txt, devolve a matriz maximum alocada e grava a
 * quantidade de clientes em *quantidade_de_clientes. Devolve NULL em
 * caso de erro, ja tendo avisado no stderr.
 *
 * A matriz e um vetor unico de clientes * recursos inteiros: a linha do
 * cliente i comeca em maximum + i * quantidade_de_recursos. Um malloc,
 * um free, e a mesma forma vai servir para allocation e need na Sessao 4.
 *
 * Sao duas passadas pelo arquivo porque alocar exige saber quantos
 * clientes existem, e saber isso exige ler o arquivo. A primeira passada
 * conta as linhas e confere a largura de cada uma, rewind volta ao
 * comeco, e a segunda converte os valores. De quebra isso atende o
 * "checar todos os parametros antes de aceita-los": nada e alocado antes
 * de a largura do arquivo inteiro estar validada.
 */
static int *ler_customer(int quantidade_de_recursos,
                         int *quantidade_de_clientes) {
    FILE *arquivo = NULL;
    char linha[TAMANHO_MAXIMO_DA_LINHA];
    int *maximum = NULL;
    int clientes = 0;
    int numero_da_linha = 0;
    int cliente = 0;

    arquivo = fopen(ARQUIVO_CUSTOMER, "r");
    if (arquivo == NULL) {
        fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO);
        fprintf(stderr, "  (nao foi possivel abrir o arquivo)\n");
        return NULL;
    }

    /* Primeira passada: conta os clientes e confere a largura das linhas. */
    while (fgets(linha, TAMANHO_MAXIMO_DA_LINHA, arquivo) != NULL) {
        int tamanho;
        int valores;

        numero_da_linha++;

        /* Linha maior que o buffer: fgets parou no meio dela, entao o que
           sobrou no arquivo e o resto desta linha, nao a proxima. */
        tamanho = (int) strlen(linha);
        if (tamanho == TAMANHO_MAXIMO_DA_LINHA - 1 &&
            linha[tamanho - 1] != '\n') {
            fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO);
            fprintf(stderr, "  (linha %d longa demais)\n", numero_da_linha);
            fclose(arquivo);
            return NULL;
        }

        remover_fim_de_linha(linha);
        if (linha_em_branco(linha)) {
            continue;
        }

        /* Mesma checagem resolve os dois casos: largura diferente de
           argc - 1 e larguras diferentes entre si, porque todas sao
           comparadas com o mesmo numero de recursos. */
        valores = contar_valores_na_linha(linha);
        if (valores != quantidade_de_recursos) {
            fprintf(stderr, ERRO_QUANTIDADE_DE_RECURSOS);
            fprintf(stderr, "  (linha %d declara %d valores, esperados %d)\n",
                    numero_da_linha, valores, quantidade_de_recursos);
            fclose(arquivo);
            return NULL;
        }

        clientes++;
    }

    if (clientes == 0) {         /* arquivo vazio ou so com linhas em branco */
        fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO);
        fprintf(stderr, "  (nenhum cliente encontrado)\n");
        fclose(arquivo);
        return NULL;
    }

    maximum = malloc((size_t) clientes * (size_t) quantidade_de_recursos *
                     sizeof(int));
    if (maximum == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria para %d clientes.\n",
                clientes);
        fclose(arquivo);
        return NULL;
    }

    /* Segunda passada: converte os valores de cada linha. */
    rewind(arquivo);
    numero_da_linha = 0;

    while (cliente < clientes &&
           fgets(linha, TAMANHO_MAXIMO_DA_LINHA, arquivo) != NULL) {
        numero_da_linha++;

        remover_fim_de_linha(linha);
        if (linha_em_branco(linha)) {
            continue;
        }

        if (ler_valores_da_linha(linha,
                                 maximum + cliente * quantidade_de_recursos,
                                 quantidade_de_recursos) != 0) {
            fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO);
            fprintf(stderr, "  (linha %d: esperados %d inteiros nao "
                            "negativos separados por virgula)\n",
                    numero_da_linha, quantidade_de_recursos);
            free(maximum);
            fclose(arquivo);
            return NULL;
        }

        cliente++;
    }

    fclose(arquivo);
    *quantidade_de_clientes = clientes;
    return maximum;
}

int main(int argc, char *argv[]) {
    /* Cada argumento depois do nome do programa e o total de instancias
       de um recurso. */
    int quantidade_de_recursos = argc - 1;
    int *total_de_instancias = NULL;
    int *maximum = NULL;
    int quantidade_de_clientes = 0;
    int i;
    int j;

    if (quantidade_de_recursos < 1) {
        fprintf(stderr, "Uso: %s <total_recurso_0> [total_recurso_1] ...\n",
                argv[0]);
        fprintf(stderr, "Informe o total de instancias de cada recurso "
                        "(inteiros nao negativos).\n");
        fprintf(stderr, "Exemplo: %s 10 5 7\n", argv[0]);
        return 1;
    }

    total_de_instancias = malloc((size_t) quantidade_de_recursos * sizeof(int));
    if (total_de_instancias == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria para %d recursos.\n",
                quantidade_de_recursos);
        return 1;
    }

    for (i = 0; i < quantidade_de_recursos; i++) {
        if (converter_inteiro_nao_negativo(argv[i + 1],
                                           &total_de_instancias[i]) != 0) {
            fprintf(stderr, "Erro: argumento %d (\"%s\") nao e um inteiro "
                            "nao negativo valido.\n", i + 1, argv[i + 1]);
            free(total_de_instancias);
            return 1;
        }
    }

    maximum = ler_customer(quantidade_de_recursos, &quantidade_de_clientes);
    if (maximum == NULL) {
        free(total_de_instancias);
        return 1;
    }

    /* TODO (sessao 3): ler o commands.txt. */
    /* TODO (sessao 4): allocation zerada e need = maximum - allocation. */

    /* Saida temporaria de conferencia: sai na Sessao 7, quando o
       result.txt passa a ser a saida oficial. */
    printf("recursos: %d\n", quantidade_de_recursos);
    printf("total de instancias:");
    for (i = 0; i < quantidade_de_recursos; i++) {
        printf(" %d", total_de_instancias[i]);
    }
    printf("\n");

    printf("clientes: %d\n", quantidade_de_clientes);
    printf("maximum:\n");
    for (i = 0; i < quantidade_de_clientes; i++) {
        printf("  cliente %d:", i);
        for (j = 0; j < quantidade_de_recursos; j++) {
            printf(" %d", maximum[i * quantidade_de_recursos + j]);
        }
        printf("\n");
    }

    free(maximum);
    free(total_de_instancias);
    return 0;
}

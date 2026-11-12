/*
 * banker.c - Algoritmo do Banqueiro (Implementacao 6)
 *
 * Sessao 1: validacao dos argumentos de linha de comando.
 * Cada argumento depois do nome do programa e o total de instancias
 * de um recurso do sistema.
 *
 * Uso: ./banker <total_r0> <total_r1> ... <total_rN>
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char *argv[]) {
    /* Cada argumento depois do nome do programa e o total de instancias
       de um recurso. */
    int quantidade_de_recursos = argc - 1;
    int *total_de_instancias = NULL;
    int i;

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

    /* TODO (sessao 2): ler o customer.txt e montar a matriz maximum. */
    /* TODO (sessao 3): ler o commands.txt. */

    printf("recursos: %d\n", quantidade_de_recursos);
    printf("total de instancias:");
    for (i = 0; i < quantidade_de_recursos; i++) {
        printf(" %d", total_de_instancias[i]);
    }
    printf("\n");

    free(total_de_instancias);
    return 0;
}

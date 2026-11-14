/*
 * banker.c - Algoritmo do Banqueiro (Implementacao 6)
 *
 * Sessao 1: validacao dos argumentos de linha de comando.
 * Cada argumento depois do nome do programa e o total de instancias
 * de um recurso do sistema.
 *
 * Sessao 2: leitura do customer.txt e montagem da matriz maximum.
 *
 * Sessao 3: leitura e validacao do commands.txt. Esta sessao so valida:
 * a execucao dos comandos entra na Sessao 5.
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
#define ARQUIVO_COMMANDS "commands.txt"

/* Os campos de uma linha do commands.txt sao separados por espaco, e nao
   por virgula como no customer.txt. A tabulacao entra na lista porque e o
   outro jeito de um editor escrever a mesma separacao. */
#define SEPARADORES_DO_COMANDO " \t"

/* Limita o comprimento de UMA linha do arquivo, nao a quantidade de
   clientes nem a de recursos: essas duas continuam descobertas em tempo
   de execucao, como o enunciado exige. */
#define TAMANHO_MAXIMO_DA_LINHA 1024

/* As duas mensagens de erro previstas no enunciado. O %s recebe o nome do
   arquivo: as mesmas duas mensagens valem para o customer.txt e para o
   commands.txt, e quem le o erro precisa saber de qual dos dois se trata.
   TODO: trocar o texto abaixo pelo texto literal do enunciado. */
#define ERRO_QUANTIDADE_DE_RECURSOS                                          \
    "Erro: a quantidade de recursos informada na linha de comando e "        \
    "diferente da quantidade de recursos do arquivo %s.\n"
#define ERRO_FORMATO_DO_ARQUIVO                                              \
    "Erro: o arquivo %s nao esta no formato indicado.\n"

/* Resultados de ler_comando. Sao tres e nao dois porque os dois erros do
   enunciado sao diferentes: quantidade de valores errada e o erro (1), e
   qualquer outro defeito de forma e o erro (2). */
#define COMANDO_OK 0
#define COMANDO_FORMATO_INVALIDO (-1)
#define COMANDO_QUANTIDADE_INVALIDA (-2)

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
        fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_CUSTOMER);
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
            fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_CUSTOMER);
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
            fprintf(stderr, ERRO_QUANTIDADE_DE_RECURSOS, ARQUIVO_CUSTOMER);
            fprintf(stderr, "  (linha %d declara %d valores, esperados %d)\n",
                    numero_da_linha, valores, quantidade_de_recursos);
            fclose(arquivo);
            return NULL;
        }

        clientes++;
    }

    if (clientes == 0) {         /* arquivo vazio ou so com linhas em branco */
        fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_CUSTOMER);
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
            fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_CUSTOMER);
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

/*
 * Conta os campos de uma linha de comando, tratando qualquer sequencia de
 * espacos e tabulacoes como um separador so.
 *
 * Existe separada de ler_comando porque strtok escreve na linha: contar os
 * campos antes de tokenizar e o que permite dizer, na mensagem de erro,
 * quantos valores a linha declarava quando a quantidade esta errada.
 */
static int contar_campos_na_linha(const char *linha) {
    int campos = 0;
    int dentro_de_um_campo = 0;
    int i;

    for (i = 0; linha[i] != '\0'; i++) {
        if (linha[i] == ' ' || linha[i] == '\t') {
            dentro_de_um_campo = 0;
        } else if (!dentro_de_um_campo) {
            dentro_de_um_campo = 1;
            campos++;
        }
    }
    return campos;
}

/*
 * Entende UMA linha do commands.txt.
 *
 * Grava em *tipo o caractere 'Q' (RQ), 'L' (RL) ou '*'; em *cliente o indice
 * do cliente; e em valores[0..quantidade_de_recursos-1] os valores pedidos ou
 * liberados. Na linha "*" nem *cliente nem valores sao tocados.
 *
 * Devolve COMANDO_OK, COMANDO_QUANTIDADE_INVALIDA (RQ/RL com quantidade de
 * valores diferente da quantidade de recursos) ou COMANDO_FORMATO_INVALIDO.
 *
 * Escreve na linha, porque strtok troca cada separador por '\0'. E o unico
 * lugar do programa que sabe a forma de um comando: serve a validacao desta
 * sessao e vai servir a execucao na Sessao 5, sem um segundo parser.
 */
static int ler_comando(char *linha, char *tipo, int *cliente, int *valores,
                       int quantidade_de_recursos) {
    int campos = contar_campos_na_linha(linha);
    char *campo = NULL;
    int i;

    campo = strtok(linha, SEPARADORES_DO_COMANDO);
    if (campo == NULL) {
        return COMANDO_FORMATO_INVALIDO;   /* linha sem campo nenhum */
    }

    if (strcmp(campo, "*") == 0) {
        /* O "*" e um comando sozinho: qualquer campo depois dele e erro. */
        if (campos != 1) {
            return COMANDO_FORMATO_INVALIDO;
        }
        *tipo = '*';
        return COMANDO_OK;
    }

    if (strcmp(campo, "RQ") == 0) {
        *tipo = 'Q';
    } else if (strcmp(campo, "RL") == 0) {
        *tipo = 'L';
    } else {
        return COMANDO_FORMATO_INVALIDO;   /* nao e RQ, nem RL, nem * */
    }

    /* Uma linha RQ/RL tem exatamente 2 + quantidade_de_recursos campos: o
       comando, o cliente e um valor por recurso.

       Menos de dois campos e um RQ/RL sem nem o indice do cliente, defeito de
       forma e nao de quantidade de valores; de dois campos em diante, a
       diferenca de largura e o erro (1) do enunciado. Conferir a largura antes
       de converter campo nenhum tambem garante que os strtok seguintes nao
       devolvem NULL. */
    if (campos < 2) {
        return COMANDO_FORMATO_INVALIDO;
    }
    if (campos != 2 + quantidade_de_recursos) {
        return COMANDO_QUANTIDADE_INVALIDA;
    }

    campo = strtok(NULL, SEPARADORES_DO_COMANDO);
    if (converter_inteiro_nao_negativo(campo, cliente) != 0) {
        return COMANDO_FORMATO_INVALIDO;
    }

    for (i = 0; i < quantidade_de_recursos; i++) {
        campo = strtok(NULL, SEPARADORES_DO_COMANDO);
        if (converter_inteiro_nao_negativo(campo, &valores[i]) != 0) {
            return COMANDO_FORMATO_INVALIDO;
        }
    }

    return COMANDO_OK;
}

/*
 * Le o commands.txt inteiro conferindo a forma de cada comando, sem executar
 * nada. Devolve 0 se o arquivo todo esta valido e -1 no primeiro erro, ja
 * tendo avisado no stderr.
 *
 * O enunciado manda checar todos os parametros antes de aceita-los. A leitura
 * mais segura disso e validar o arquivo inteiro antes do primeiro comando: se
 * o comando 9 estiver malformado, o programa nao deveria ter alocado nada nos
 * comandos 1 a 8 e deixado um result.txt pela metade. Por isso a validacao e
 * uma passada propria, e a execucao (Sessao 5) vai reler o arquivo do zero em
 * vez de guardar os comandos em memoria.
 *
 * Duas decisoes minhas, que o enunciado nao cobre:
 *
 * - indice de cliente fora da faixa (um "RQ 9 ..." com cinco clientes) e
 *   tratado como erro de formato, porque nao existe nada razoavel a fazer com
 *   esse comando e aceitar em silencio esconderia um arquivo de entrada
 *   errado;
 * - commands.txt sem comando nenhum e aceito, e vai gerar um result.txt
 *   vazio. Zero comandos e uma entrada degenerada mas coerente, e recusar
 *   seria inventar uma regra.
 *
 * Um RL que libera mais do que o cliente tem alocado nao e erro de formato, e
 * erro de estado: so da para saber na hora de executar, e fica para a
 * Sessao 5.
 */
static int validar_commands(int quantidade_de_recursos,
                            int quantidade_de_clientes) {
    FILE *arquivo = NULL;
    char linha[TAMANHO_MAXIMO_DA_LINHA];
    int *valores = NULL;
    int numero_da_linha = 0;
    int comandos = 0;
    int i;

    arquivo = fopen(ARQUIVO_COMMANDS, "r");
    if (arquivo == NULL) {
        fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_COMMANDS);
        fprintf(stderr, "  (nao foi possivel abrir o arquivo)\n");
        return -1;
    }

    /* Um vetor de valores basta para todas as linhas, e o malloc fica fora do
       laco: alocar e liberar a cada linha funcionaria, mas nao tem motivo. */
    valores = malloc((size_t) quantidade_de_recursos * sizeof(int));
    if (valores == NULL) {
        fprintf(stderr, "Erro: falha ao alocar memoria para %d recursos.\n",
                quantidade_de_recursos);
        fclose(arquivo);
        return -1;
    }

    /* Saida temporaria de conferencia: sai na Sessao 7, quando o result.txt
       passa a ser a saida oficial. */
    printf("comandos de %s:\n", ARQUIVO_COMMANDS);

    while (fgets(linha, TAMANHO_MAXIMO_DA_LINHA, arquivo) != NULL) {
        char tipo = 0;
        int cliente = 0;
        int tamanho;
        int campos;
        int resultado;

        numero_da_linha++;

        /* Linha maior que o buffer: fgets parou no meio dela, entao o que
           sobrou no arquivo e o resto desta linha, nao a proxima. */
        tamanho = (int) strlen(linha);
        if (tamanho == TAMANHO_MAXIMO_DA_LINHA - 1 &&
            linha[tamanho - 1] != '\n') {
            fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_COMMANDS);
            fprintf(stderr, "  (linha %d longa demais)\n", numero_da_linha);
            free(valores);
            fclose(arquivo);
            return -1;
        }

        remover_fim_de_linha(linha);
        if (linha_em_branco(linha)) {
            continue;
        }

        campos = contar_campos_na_linha(linha);
        resultado = ler_comando(linha, &tipo, &cliente, valores,
                                quantidade_de_recursos);

        if (resultado == COMANDO_QUANTIDADE_INVALIDA) {
            fprintf(stderr, ERRO_QUANTIDADE_DE_RECURSOS, ARQUIVO_COMMANDS);
            fprintf(stderr, "  (linha %d declara %d valores, esperados %d)\n",
                    numero_da_linha, campos - 2, quantidade_de_recursos);
            free(valores);
            fclose(arquivo);
            return -1;
        }

        if (resultado != COMANDO_OK) {
            fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_COMMANDS);
            fprintf(stderr, "  (linha %d: esperado \"RQ <cliente> <%d "
                            "valores>\", \"RL <cliente> <%d valores>\" ou "
                            "\"*\", com inteiros nao negativos)\n",
                    numero_da_linha, quantidade_de_recursos,
                    quantidade_de_recursos);
            free(valores);
            fclose(arquivo);
            return -1;
        }

        /* Decisao minha: cliente inexistente e erro de formato. */
        if (tipo != '*' &&
            (cliente < 0 || cliente >= quantidade_de_clientes)) {
            fprintf(stderr, ERRO_FORMATO_DO_ARQUIVO, ARQUIVO_COMMANDS);
            fprintf(stderr, "  (linha %d: cliente %d nao existe; o %s declara "
                            "%d clientes, de 0 a %d)\n",
                    numero_da_linha, cliente, ARQUIVO_CUSTOMER,
                    quantidade_de_clientes, quantidade_de_clientes - 1);
            free(valores);
            fclose(arquivo);
            return -1;
        }

        comandos++;

        /* Saida temporaria de conferencia, so desta sessao. */
        if (tipo == '*') {
            printf("  linha %d: * (imprimir o estado)\n", numero_da_linha);
        } else {
            printf("  linha %d: %s cliente %d, valores:", numero_da_linha,
                   tipo == 'Q' ? "RQ" : "RL", cliente);
            for (i = 0; i < quantidade_de_recursos; i++) {
                printf(" %d", valores[i]);
            }
            printf("\n");
        }
    }

    printf("comandos validados: %d\n", comandos);

    free(valores);
    fclose(arquivo);
    return 0;
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

    /* O commands.txt e validado inteiro aqui, antes de qualquer execucao:
       nenhum comando roda enquanto o arquivo todo nao estiver aceito. */
    if (validar_commands(quantidade_de_recursos,
                         quantidade_de_clientes) != 0) {
        free(maximum);
        free(total_de_instancias);
        return 1;
    }

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

# Algoritmo do Banqueiro

Implementação em C do algoritmo do banqueiro (Banker's algorithm) descrito no
projeto da página P-45 do livro *Operating System Concepts*, de Silberschatz,
10ª edição. Diferente da versão do livro, o número de clientes e o número de
tipos de recurso não são fixos: os dois vêm da entrada.

## Compilação

```
make
```

Gera o executável `banker`. Para remover os arquivos compilados:

```
make clean
```

## Execução

```
./banker <total_recurso_0> [total_recurso_1] ...
```

Cada argumento é o número total de instâncias de um tipo de recurso, e deve
haver um argumento por tipo. Exemplo, com três tipos de recurso:

```
./banker 10 5 7
```

### Arquivos de entrada

Os dois arquivos precisam estar no diretório de onde o programa é executado.

**`customer.txt`** — a matriz *maximum*: uma linha por cliente, com os valores
separados por vírgula. O número de linhas define a quantidade de clientes.

```
7,5,3
3,2,2
9,0,2
```

**`commands.txt`** — um comando por linha, com os campos separados por espaço:

- `RQ <cliente> <valores>` — o cliente pede recursos;
- `RL <cliente> <valores>` — o cliente devolve recursos;
- `*` — imprime o estado atual (maximum, allocation, need e available).

```
RQ 0 0 2 1
RL 0 0 1 1
*
```

Os dois arquivos podem usar fim de linha Unix (LF) ou Windows (CRLF) e podem
terminar sem quebra de linha na última linha.

### Saída

O resultado é gravado em `result.txt`, criado no diretório de execução: as
mensagens de alocação, liberação e negação de cada comando e a tabela de estado
de cada `*`. O programa não imprime nada na saída padrão; erros e avisos vão
para o `stderr`.

## Tratamento de erros

O programa trata os dois casos de erro pedidos no enunciado:

- a quantidade de argumentos na linha de comando é diferente da quantidade de
  recursos declarada nos arquivos;
- um arquivo de entrada não existe, não pode ser lido ou está fora do formato.

Em todos esses casos o programa escreve uma mensagem no `stderr`, termina com
código de saída 1 e não cria o `result.txt`.

## Limitações conhecidas

- **Alinhamento da tabela do `*`.** As larguras das colunas reproduzem o exemplo
  do enunciado, que tem três recursos. Com outra quantidade de recursos as
  barras saem do lugar: nenhuma informação se perde, mas o alinhamento quebra
  (testado com oito recursos).
- **Maximum maior que o total do banco.** Se o maximum de algum cliente passar
  do total de algum recurso, esse cliente nunca consegue terminar e o estado é
  inseguro desde o início. Por isso todo `RQ` é negado, inclusive um pedido de
  zero. O comportamento é fiel ao algoritmo; o programa não valida essa condição
  na entrada porque o enunciado não pede.
- **`RL` maior que o alocado.** Um `RL` que devolve mais do que o cliente tem
  alocado gera um aviso no `stderr` e é ignorado: não altera o estado nem
  escreve no `result.txt`. O enunciado não define esse caso.
- **Casos não previstos no enunciado.** Um índice de cliente fora da faixa é
  tratado como erro de formato. Um `commands.txt` vazio é aceito e produz um
  `result.txt` vazio.
- **Tamanho das linhas.** Cada linha dos arquivos de entrada pode ter no máximo
  1024 caracteres, o que comporta cerca de 250 recursos por linha. Não há limite
  para a quantidade de clientes nem de recursos: a memória é alocada conforme a
  entrada.

## Ambiente de teste

- WSL2 com Ubuntu 24.04 em aarch64
- gcc 13.3.0, com `-Wall -Wextra -std=c11`, sem avisos

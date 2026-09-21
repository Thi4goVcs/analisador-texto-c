# analisador-texto-c

Programa de terminal em C que lê um arquivo de texto e mostra quantas linhas, palavras e caracteres ele tem, quais são as palavras que mais aparecem e, se você pedir, em quais linhas aparece uma palavra ou frase.

É o primeiro projeto em C do portfólio. Escolhi algo que parece um `wc` misturado com `grep` porque dá pra treinar o básico que C cobra de verdade: ler arquivo, alocar e liberar memória, mexer com string e fazer uma tabela hash na mão, já que C não tem dicionário pronto.

## Compilando e rodando

Precisa de `gcc` (testei com o MinGW-w64 no Windows).

```bash
git clone https://github.com/Thi4goVcs/analisador-texto-c.git
cd analisador-texto-c
make
./analisador exemplos/exemplo.txt
```

Sem `make`, é só:

```bash
gcc -Wall -Wextra -std=c11 -O2 -o analisador main.c
```

## Opções

```
-b, --buscar TERMO    mostra as linhas que têm a palavra ou frase
-i, --ignorar-caixa   busca sem diferenciar maiúscula de minúscula
-t, --top N           quantas palavras mostrar no ranking (padrão: 5)
-h, --ajuda           mostra a ajuda
```

Exemplo:

```bash
./analisador exemplos/exemplo.txt --buscar "erro de" --top 3
```

## Testes

```bash
make test
```

O `testes.sh` compara a saída do programa com o `wc` (linhas, palavras, bytes) e com o `grep` (busca) em cinco arquivos de exemplo, incluindo arquivo vazio, sem quebra de linha no final e com fim de linha do Windows (CRLF). Também confere que erros de uso saem com código 1.

## Coisas que decidi (e limites que conheço)

- Contagem de linhas segue o `wc`: conta os `\n`. Um arquivo sem quebra no final tem uma linha a menos do que você "enxerga".
- "Caracteres" conta letras de verdade em UTF-8 (o `ã` vale 1, não 2). "Bytes" mostra o tamanho real.
- No ranking, letras acentuadas maiúsculas e minúsculas não são juntadas (`É` e `é` contam como palavras diferentes). Só as letras ASCII são convertidas pra minúscula. A busca com `-i` tem a mesma limitação.
- A frase buscada precisa estar inteira na mesma linha.
- O arquivo inteiro é lido pra memória. Serve pra arquivo de texto normal, não pra arquivo de vários gigas.
- Se o arquivo tiver byte nulo no meio, o programa para de ler o texto ali na hora de buscar e montar o ranking.

/* analisador-texto-c: estatisticas, ranking de palavras e busca em arquivo de texto. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_TABELA 4096

typedef struct No {
    char *palavra;
    int contagem;
    struct No *prox;
} No;

typedef struct {
    size_t linhas;
    size_t palavras;
    size_t caracteres;
    size_t bytes;
} Estatisticas;

static No *tabela[TAM_TABELA];

static void uso(FILE *saida) {
    fputs("Uso: analisador ARQUIVO [opcoes]\n"
          "  -b, --buscar TERMO    mostra as linhas com a palavra ou frase\n"
          "  -i, --ignorar-caixa   busca sem diferenciar maiusculas de minusculas\n"
          "  -t, --top N           quantas palavras mostrar no ranking (padrao: 5)\n"
          "  -h, --ajuda           mostra esta mensagem\n",
          saida);
}

/* Le o arquivo inteiro pra memoria. O buffer cresce dobrando, e fica com 1 byte
 * extra pro '\0' final, pra poder usar funcoes de string em cima dele. */
static char *ler_arquivo(const char *caminho, size_t *tamanho) {
    FILE *f = fopen(caminho, "rb");
    if (!f) return NULL;

    size_t cap = 4096, len = 0, lidos;
    char *buf = malloc(cap + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    while ((lidos = fread(buf + len, 1, cap - len, f)) > 0) {
        len += lidos;
        if (len == cap) {
            cap *= 2;
            char *novo = realloc(buf, cap + 1);
            if (!novo) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = novo;
        }
    }
    fclose(f);

    buf[len] = '\0';
    *tamanho = len;
    return buf;
}

/* Mesma regra do wc: linha e' contada pelo '\n'. Caractere conta so' os bytes
 * que nao sao continuacao UTF-8 (10xxxxxx), senao "ã" valeria 2. */
static Estatisticas contar(const char *texto, size_t n) {
    Estatisticas e = {0, 0, 0, n};
    int em_palavra = 0;

    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)texto[i];
        if (c == '\n') e.linhas++;
        if ((c & 0xC0) != 0x80) e.caracteres++;
        if (isspace(c)) {
            em_palavra = 0;
        } else if (!em_palavra) {
            em_palavra = 1;
            e.palavras++;
        }
    }
    return e;
}

static unsigned long hash(const char *s) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++)) h = h * 33 + (unsigned long)c;
    return h;
}

/* Bytes >= 0x80 entram como letra pra "não" e "ação" ficarem inteiras (UTF-8). */
static int eh_letra(unsigned char c) {
    return isalnum(c) || c >= 0x80;
}

static void registrar(const char *inicio, size_t len) {
    char *p = malloc(len + 1);
    if (!p) return;
    for (size_t i = 0; i < len; i++) p[i] = (char)tolower((unsigned char)inicio[i]);
    p[len] = '\0';

    unsigned long h = hash(p) % TAM_TABELA;
    for (No *no = tabela[h]; no; no = no->prox) {
        if (strcmp(no->palavra, p) == 0) {
            no->contagem++;
            free(p);
            return;
        }
    }

    No *novo = malloc(sizeof(No));
    if (!novo) {
        free(p);
        return;
    }
    novo->palavra = p;
    novo->contagem = 1;
    novo->prox = tabela[h];
    tabela[h] = novo;
}

static void tokenizar(const char *texto) {
    const char *p = texto;
    while (*p) {
        while (*p && !eh_letra((unsigned char)*p)) p++;
        const char *ini = p;
        while (*p && eh_letra((unsigned char)*p)) p++;
        if (p > ini) registrar(ini, (size_t)(p - ini));
    }
}

static int comparar(const void *a, const void *b) {
    const No *x = *(const No *const *)a;
    const No *y = *(const No *const *)b;
    if (y->contagem != x->contagem) return y->contagem - x->contagem;
    return strcmp(x->palavra, y->palavra);
}

static size_t mostrar_ranking(long top) {
    size_t total = 0;
    for (size_t i = 0; i < TAM_TABELA; i++)
        for (No *no = tabela[i]; no; no = no->prox) total++;

    No **lista = malloc((total ? total : 1) * sizeof(No *));
    if (!lista) return total;

    size_t k = 0;
    for (size_t i = 0; i < TAM_TABELA; i++)
        for (No *no = tabela[i]; no; no = no->prox) lista[k++] = no;
    qsort(lista, total, sizeof(No *), comparar);

    printf("\nTop %ld palavras:\n", top);
    for (size_t i = 0; i < total && i < (size_t)top; i++)
        printf("  %zu. %s (%d)\n", i + 1, lista[i]->palavra, lista[i]->contagem);

    free(lista);
    return total;
}

static void liberar_tabela(void) {
    for (size_t i = 0; i < TAM_TABELA; i++) {
        No *no = tabela[i];
        while (no) {
            No *prox = no->prox;
            free(no->palavra);
            free(no);
            no = prox;
        }
        tabela[i] = NULL;
    }
}

static int iguais(char a, char b, int ignorar_caixa) {
    if (ignorar_caixa) return tolower((unsigned char)a) == tolower((unsigned char)b);
    return a == b;
}

/* Procura agulha em palha. So' ignora caixa de letras ASCII: "É" e "é" continuam
 * diferentes, porque em UTF-8 sao sequencias de bytes distintas. */
static const char *achar(const char *palha, const char *agulha, int ignorar_caixa) {
    size_t n = strlen(agulha);
    if (n == 0) return NULL;

    for (; *palha; palha++) {
        size_t i = 0;
        while (i < n && palha[i] && iguais(palha[i], agulha[i], ignorar_caixa)) i++;
        if (i == n) return palha;
    }
    return NULL;
}

static void buscar(const char *texto, const char *termo, int ignorar_caixa) {
    size_t total = 0, linhas_com = 0, numero = 1;
    size_t tam_termo = strlen(termo);

    printf("\nBusca por \"%s\"%s:\n", termo, ignorar_caixa ? " (ignorando maiusculas)" : "");

    const char *ini = texto;
    while (*ini) {
        const char *fim = strchr(ini, '\n');
        size_t len = fim ? (size_t)(fim - ini) : strlen(ini);

        char *linha = malloc(len + 1);
        if (!linha) return;
        memcpy(linha, ini, len);
        linha[len] = '\0';
        if (len > 0 && linha[len - 1] == '\r') linha[len - 1] = '\0';

        /* avanca pelo tamanho do termo pra nao contar ocorrencias sobrepostas */
        size_t na_linha = 0;
        for (const char *p = achar(linha, termo, ignorar_caixa); p;
             p = achar(p + tam_termo, termo, ignorar_caixa))
            na_linha++;

        if (na_linha > 0) {
            printf("  linha %zu: %s\n", numero, linha);
            total += na_linha;
            linhas_com++;
        }
        free(linha);

        ini = fim ? fim + 1 : ini + len;
        numero++;
    }

    if (total == 0) puts("  nenhuma ocorrencia.");
    printf("Ocorrencias: %zu\n", total);
    printf("Linhas com ocorrencia: %zu\n", linhas_com);
}

int main(int argc, char **argv) {
    const char *caminho = NULL, *termo = NULL;
    int ignorar_caixa = 0;
    long top = 5;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--ajuda")) {
            uso(stdout);
            return 0;
        } else if (!strcmp(a, "-i") || !strcmp(a, "--ignorar-caixa")) {
            ignorar_caixa = 1;
        } else if (!strcmp(a, "-b") || !strcmp(a, "--buscar")) {
            if (++i >= argc || argv[i][0] == '\0') {
                fputs("erro: --buscar precisa de um termo nao vazio\n", stderr);
                return 1;
            }
            termo = argv[i];
        } else if (!strcmp(a, "-t") || !strcmp(a, "--top")) {
            char *resto;
            if (++i >= argc || (top = strtol(argv[i], &resto, 10), *resto != '\0' || top <= 0)) {
                fputs("erro: --top precisa de um numero maior que zero\n", stderr);
                return 1;
            }
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "erro: opcao desconhecida: %s\n", a);
            uso(stderr);
            return 1;
        } else if (!caminho) {
            caminho = a;
        } else {
            fputs("erro: so' um arquivo por vez\n", stderr);
            return 1;
        }
    }

    if (!caminho) {
        uso(stderr);
        return 1;
    }

    size_t tamanho = 0;
    char *texto = ler_arquivo(caminho, &tamanho);
    if (!texto) {
        fprintf(stderr, "erro: nao consegui abrir \"%s\"\n", caminho);
        return 1;
    }

    Estatisticas e = contar(texto, tamanho);
    tokenizar(texto);

    printf("Arquivo: %s\n", caminho);
    printf("Linhas: %zu\n", e.linhas);
    printf("Palavras: %zu\n", e.palavras);
    printf("Caracteres: %zu\n", e.caracteres);
    printf("Bytes: %zu\n", e.bytes);
    size_t unicas = mostrar_ranking(top);
    printf("Palavras unicas: %zu\n", unicas);

    if (termo) buscar(texto, termo, ignorar_caixa);

    liberar_tabela();
    free(texto);
    return 0;
}

#!/usr/bin/env bash
# Compara o analisador com o wc e o grep. Uso: make test (ou bash testes.sh)

BIN=./analisador
[ -x "$BIN.exe" ] && BIN=./analisador.exe

falhas=0

confere() {
  # $1 = descricao, $2 = esperado, $3 = obtido
  if [ "$2" = "$3" ]; then
    echo "ok    $1"
  else
    echo "FALHA $1 (esperado '$2', veio '$3')"
    falhas=$((falhas + 1))
  fi
}

campo() { "$BIN" "$1" | grep "^$2:" | head -1 | awk '{print $NF}'; }

for f in exemplos/*.txt; do
  confere "$f linhas"   "$(wc -l < "$f" | tr -d ' ')" "$(campo "$f" Linhas)"
  confere "$f palavras" "$(wc -w < "$f" | tr -d ' ')" "$(campo "$f" Palavras)"
  confere "$f bytes"    "$(wc -c < "$f" | tr -d ' ')" "$(campo "$f" Bytes)"
done

f=exemplos/exemplo.txt

# busca sensivel a caixa: so' "programa" minusculo
esperado=$(grep -o -F "programa" "$f" | wc -l | tr -d ' ')
confere "busca 'programa' (ocorrencias)" "$esperado" "$("$BIN" "$f" -b programa | grep '^Ocorrencias:' | awk '{print $NF}')"

# ignorando caixa: pega "Programa" tambem
# (sem -F: o grep 3.0 do Git Bash aborta com -i -F; o termo nao tem metacaractere de regex)
esperado=$(grep -o -i "programa" "$f" | wc -l | tr -d ' ')
confere "busca -i 'programa' (ocorrencias)" "$esperado" "$("$BIN" "$f" -b programa -i | grep '^Ocorrencias:' | awk '{print $NF}')"

esperado=$(grep -c -i "programa" "$f")
confere "busca -i 'programa' (linhas)" "$esperado" "$("$BIN" "$f" -b programa -i | grep '^Linhas com ocorrencia:' | awk '{print $NF}')"

# frase com espaco
esperado=$(grep -o -F "erro de" "$f" | wc -l | tr -d ' ')
confere "busca frase 'erro de'" "$esperado" "$("$BIN" "$f" -b "erro de" | grep '^Ocorrencias:' | awk '{print $NF}')"

# sem resultado
confere "busca sem resultado" "0" "$("$BIN" "$f" -b zzzxyz | grep '^Ocorrencias:' | awk '{print $NF}')"

# CRLF: a linha impressa nao pode carregar o \r
if "$BIN" exemplos/crlf.txt -b CRLF | grep -q $'\r'; then
  echo "FALHA crlf: saida ainda tem \\r"; falhas=$((falhas + 1))
else
  echo "ok    crlf sem \\r na saida"
fi

# erros de uso devem sair com codigo 1
"$BIN" arquivo_que_nao_existe.txt 2>/dev/null; confere "arquivo inexistente -> codigo 1" "1" "$?"
"$BIN" "$f" --top 0 2>/dev/null;                confere "--top 0 -> codigo 1" "1" "$?"
"$BIN" "$f" --buscar 2>/dev/null;               confere "--buscar sem termo -> codigo 1" "1" "$?"

echo
[ "$falhas" -eq 0 ] && echo "todos os testes passaram" || echo "$falhas falha(s)"
exit "$falhas"

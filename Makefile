CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

analisador: main.c
	$(CC) $(CFLAGS) -o analisador main.c

test: analisador
	bash testes.sh

clean:
	rm -f analisador analisador.exe

.PHONY: test clean

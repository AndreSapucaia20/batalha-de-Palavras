# ============================================================
# Makefile — Batalha de Palavras em Rede
#
# Uso:
#   make          → compila servidor e cliente
#   make clean    → remove binários
#   make help     → mostra esta ajuda
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11

SERVIDOR = servidor
CLIENTE  = cliente
OBJS_COMUM = jogo.o

.PHONY: all clean help

all: $(SERVIDOR) $(CLIENTE)
	@echo ""
	@echo "✅ Batalha de Palavras compilado com sucesso!"
	@echo ""
	@echo "Para testar:"
	@echo "  Terminal 1:  ./servidor"
	@echo "  Terminal 2:  ./cliente"
	@echo "  Terminal 3:  ./cliente"

jogo.o: jogo.c jogo.h protocolo.h
	$(CC) $(CFLAGS) -c jogo.c -o jogo.o

$(SERVIDOR): servidor.c jogo.o protocolo.h jogo.h
	$(CC) $(CFLAGS) -o $@ servidor.c jogo.o -lpthread

$(CLIENTE): cliente.c jogo.o protocolo.h jogo.h
	$(CC) $(CFLAGS) -o $@ cliente.c jogo.o

clean:
	rm -f $(SERVIDOR) $(CLIENTE) $(OBJS_COMUM)
	@echo "🧹 Binários removidos."

help:
	@echo "Alvos disponíveis:"
	@echo "  make          Compila servidor e cliente"
	@echo "  make clean    Remove binários"
	@echo "  make help     Mostra esta ajuda"

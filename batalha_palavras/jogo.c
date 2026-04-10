/*
 * jogo.c — Implementação da lógica do jogo
 *
 * Contém: validação de palavras, geração de letras,
 *         funções de envio/recebimento com protocolo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "jogo.h"

char gerar_letra(void)
{
    /* Letras comuns em português — evita K, W, X, Y */
    const char letras[] = "ABCDEFGHIJLMNOPRSTV";
    int qtd = (int)(sizeof(letras) - 1);
    return letras[rand() % qtd];
}

int validar_palavra(const char *palavra, char letra)
{
    if (palavra == NULL) return 0;

    int tamanho = (int)strlen(palavra);

    /* Regra 1: tamanho mínimo */
    if (tamanho < MIN_CHARS) return 0;

    /* Regra 2: começa com a letra da rodada (case insensitive) */
    if (tolower((unsigned char)palavra[0]) != tolower((unsigned char)letra))
        return 0;

    /* Regra 3: apenas letras */
    for (int i = 0; i < tamanho; i++) {
        if (!isalpha((unsigned char)palavra[i])) return 0;
    }

    return 1;
}

int enviar_msg(int fd, const char *tipo, const char *corpo)
{
    char buf[BUFFER_SIZE];

    if (corpo != NULL && strlen(corpo) > 0)
        snprintf(buf, sizeof(buf), "%s|%s\n", tipo, corpo);
    else
        snprintf(buf, sizeof(buf), "%s|\n", tipo);

    return (int)send(fd, buf, strlen(buf), 0);
}

int receber_com_timeout(int fd, char *buf, int tamanho, int segundos)
{
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec  = segundos;
    tv.tv_usec = 0;

    int ret = select(fd + 1, &fds, NULL, NULL, &tv);

    if (ret == 0) return 0;   /* timeout */
    if (ret  < 0) return -1;  /* erro     */

    ssize_t n = recv(fd, buf, tamanho - 1, 0);
    if (n <= 0) return -1;    /* conexão fechada ou erro */

    buf[n] = '\0';

    /* Remove \n ou \r\n */
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
    char *cr = strchr(buf, '\r');
    if (cr) *cr = '\0';

    return 1;
}
/*
 * cliente.c — Cliente de Batalha de Palavras
 * Compilação: gcc -Wall -Wextra -std=c11 -o cliente cliente.c jogo.c
 * Execução:   ./cliente
 *             ./cliente 127.0.0.1 9000
 */
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "protocolo.h"
#include "jogo.h"


static void exibir_rodada(int num, char letra, int tempo)
{
    printf("\n  ╔══════════════════════════════════╗\n");
    printf("  ║        RODADA %d de %d             ║\n", num, NUM_RODADAS);
    printf("  ║  Letra: [%c]   Tempo: %d seg       ║\n", letra, tempo);
    printf("  ║  Minimo: %d caracteres            ║\n", MIN_CHARS);
    printf("  ╚══════════════════════════════════╝\n");
    printf("  Sua palavra: ");
    fflush(stdout);
}

static void exibir_placar(const char *corpo)
{
    char copia[BUFFER_SIZE];
    strncpy(copia, corpo, BUFFER_SIZE - 1);
    copia[BUFFER_SIZE - 1] = '\0';

    char *nome1 = strtok(copia, "|");
    char *pts1  = strtok(NULL,  "|");
    char *nome2 = strtok(NULL,  "|");
    char *pts2  = strtok(NULL,  "|");

    if (nome1 && pts1 && nome2 && pts2) {
        printf("\n  ┌────────────────────────────────────┐\n");
        printf("  │  PLACAR: %-8s %s  x  %s %-8s │\n",
               nome1, pts1, pts2, nome2);
        printf("  └────────────────────────────────────┘\n");
    }
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    const char *ip    = (argc >= 2) ? argv[1] : "127.0.0.1";
    int         porta = (argc >= 3) ? atoi(argv[2]) : PORTA_PADRAO;

    printf("╔══════════════════════════════════════╗\n");
    printf("║     BATALHA DE PALAVRAS — Cliente    ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("  Conectando a %s:%d...\n", ip, porta);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port   = htons(porta);

    if (inet_pton(AF_INET, ip, &srv_addr.sin_addr) <= 0) {
        perror("Endereco invalido"); close(sock_fd); exit(EXIT_FAILURE);
    }
    if (connect(sock_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
        perror("Erro ao conectar (servidor esta rodando?)");
        close(sock_fd); exit(EXIT_FAILURE);
    }
    printf("  Conectado!\n\n");

    int em_rodada     = 0;
    int ja_enviou     = 0;
    time_t inicio_rodada = 0;

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_fd,      &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = (sock_fd > STDIN_FILENO) ? sock_fd : STDIN_FILENO;

        struct timeval tv  = {1, 0};
        struct timeval *ptv = (em_rodada && !ja_enviou) ? &tv : NULL;

        int atividade = select(max_fd + 1, &read_fds, NULL, NULL, ptv);

        if (atividade == -1) {
            if (errno == EINTR) continue;
            perror("select"); break;
        }

        /* Verifica timeout de rodada */
        if (atividade == 0 && em_rodada && !ja_enviou) {
            int elapsed = (int)(time(NULL) - inicio_rodada);
            if (elapsed >= TEMPO_RODADA) {
                printf("\n  [!] Tempo esgotado!\n");
                enviar_msg(sock_fd, P_TIMEOUT, "");
                ja_enviou = 1;
            }
            continue;
        }

        /* Mensagem do servidor */
        if (FD_ISSET(sock_fd, &read_fds)) {
            char buf[BUFFER_SIZE] = {0};
            ssize_t n = recv(sock_fd, buf, BUFFER_SIZE - 1, 0);
            if (n <= 0) {
                printf("\n[!] Conexao com o servidor encerrada.\n");
                break;
            }
            buf[n] = '\0';
            char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
            char *cr = strchr(buf, '\r'); if (cr) *cr = '\0';

            if (strncmp(buf, "MSG|", 4) == 0) {
                printf("\n  🎮 %s\n", buf + 4);
            }
            else if (strncmp(buf, "NOME|", 5) == 0) {
                char nome[NOME_SIZE] = {0};
                printf("  Digite seu nome: ");
                fflush(stdout);
                if (fgets(nome, NOME_SIZE, stdin) == NULL) break;
                char *nl2 = strchr(nome, '\n'); if (nl2) *nl2 = '\0';
                if (strlen(nome) == 0) strcpy(nome, "Jogador");
                enviar_msg(sock_fd, P_NOME, nome);
                printf("  Bem-vindo, %s!\n", nome);
            }
            else if (strncmp(buf, "AGUARDE|", 8) == 0) {
                printf("\n  ⏳ %s\n", buf + 8);
            }
            else if (strncmp(buf, "RODADA|", 7) == 0) {
                char copia[BUFFER_SIZE];
                strncpy(copia, buf + 7, BUFFER_SIZE - 1);
                copia[BUFFER_SIZE - 1] = '\0';
                char *s_num   = strtok(copia, "|");
                char *s_letra = strtok(NULL,  "|");
                char *s_tempo = strtok(NULL,  "|");
                if (s_num && s_letra && s_tempo) {
                    int  num   = atoi(s_num);
                    char letra = (char)toupper((unsigned char)s_letra[0]);
                    int  tempo = atoi(s_tempo);
                    em_rodada     = 1;
                    ja_enviou     = 0;
                    inicio_rodada = time(NULL);
                    exibir_rodada(num, letra, tempo);
                }
            }
            else if (strncmp(buf, "RESULTADO|", 10) == 0) {
                printf("\n  📋 %s\n", buf + 10);
                em_rodada = 0;
            }
            else if (strncmp(buf, "PLACAR|", 7) == 0) {
                exibir_placar(buf + 7);
            }
            else if (strncmp(buf, "FIM|", 4) == 0) {
                printf("\n  🏆 %s\n\n", buf + 4);
                break;
            }
        }

        /* Input do usuario */
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input[BUFFER_SIZE] = {0};
            if (fgets(input, BUFFER_SIZE, stdin) == NULL) break;
            char *nl = strchr(input, '\n'); if (nl) *nl = '\0';
            char *cr = strchr(input, '\r'); if (cr) *cr = '\0';

            if (!em_rodada || ja_enviou) continue;
            if (strlen(input) == 0) {
                printf("  Sua palavra: ");
                fflush(stdout);
                continue;
            }

            enviar_msg(sock_fd, P_PALAVRA, input);
            printf("  Enviado: \"%s\" — aguardando resultado...\n", input);
            ja_enviou = 1;
        }
    }

    close(sock_fd);
    printf("  Desconectado. Ate a proxima!\n");
    return 0;
}
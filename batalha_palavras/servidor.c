/*
 * servidor.c — Servidor de Batalha de Palavras
 * Compilação: gcc -Wall -Wextra -std=c11 -o servidor servidor.c jogo.c -lpthread
 * Execução:   ./servidor
 *             ./servidor 9000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocolo.h"
#include "jogo.h"

typedef struct {
    int  fd;
    char nome[NOME_SIZE];
    int  pontos;
} Jogador;

typedef struct {
    Jogador j[2];
    int     num_partida;
} Partida;

static pthread_mutex_t mutex_espera = PTHREAD_MUTEX_INITIALIZER;
static int   fd_esperando           = -1;
static char  nome_esperando[NOME_SIZE];
static int   contador_partidas      = 0;

static void executar_rodada(Partida *p, int num_rodada)
{
    char letra = gerar_letra();
    char corpo[64];

    snprintf(corpo, sizeof(corpo), "%d|%c|%d", num_rodada, letra, TEMPO_RODADA);
    enviar_msg(p->j[0].fd, P_RODADA, corpo);
    enviar_msg(p->j[1].fd, P_RODADA, corpo);

    printf("  [Rodada %d] Letra: %c\n", num_rodada, letra);

    char  palavra[2][BUFFER_SIZE] = {"", ""};
    int   recebeu[2]              = {0, 0};
    time_t inicio                 = time(NULL);

    while ((!recebeu[0] || !recebeu[1])) {
        int tempo_restante = TEMPO_RODADA - (int)(time(NULL) - inicio);
        if (tempo_restante <= 0) break;

        fd_set fds;
        FD_ZERO(&fds);
        if (!recebeu[0]) FD_SET(p->j[0].fd, &fds);
        if (!recebeu[1]) FD_SET(p->j[1].fd, &fds);

        int max_fd = (p->j[0].fd > p->j[1].fd) ? p->j[0].fd : p->j[1].fd;

        struct timeval tv = { tempo_restante, 0 };
        int ret = select(max_fd + 1, &fds, NULL, NULL, &tv);

        if (ret <= 0) break; /* timeout ou erro */

        for (int j = 0; j < 2; j++) {
            if (recebeu[j]) continue;
            if (!FD_ISSET(p->j[j].fd, &fds)) continue;

            char buf[BUFFER_SIZE] = {0};
            ssize_t n = recv(p->j[j].fd, buf, BUFFER_SIZE - 1, 0);
            if (n <= 0) { recebeu[j] = 1; continue; }

            buf[n] = '\0';
            char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
            char *cr = strchr(buf, '\r'); if (cr) *cr = '\0';

            if (strncmp(buf, "PALAVRA|", 8) == 0) {
                strncpy(palavra[j], buf + 8, BUFFER_SIZE - 1);
                recebeu[j] = 1;
            } else if (strncmp(buf, "TIMEOUT|", 8) == 0) {
                recebeu[j] = 1;
            }
        }
    }

    int valido[2];
    for (int j = 0; j < 2; j++)
        valido[j] = validar_palavra(palavra[j], letra);

    /* Palavras iguais entre os dois: ninguém pontua */
    int repetidas = 0;
    if (valido[0] && valido[1]) {
        char p0[BUFFER_SIZE], p1[BUFFER_SIZE];
        strncpy(p0, palavra[0], BUFFER_SIZE - 1);
        strncpy(p1, palavra[1], BUFFER_SIZE - 1);
        /* Compara em lowercase */
        for (int i = 0; p0[i]; i++) p0[i] = (char)tolower((unsigned char)p0[i]);
        for (int i = 0; p1[i]; i++) p1[i] = (char)tolower((unsigned char)p1[i]);
        if (strcmp(p0, p1) == 0) repetidas = 1;
    }

    for (int j = 0; j < 2; j++) {
        if (valido[j] && !repetidas)
            p->j[j].pontos++;
    }

    /* Monta e envia RESULTADO para cada jogador */
    for (int j = 0; j < 2; j++) {
        int outro = 1 - j;
        char resultado[BUFFER_SIZE];

        if (strlen(palavra[j]) == 0) {
            snprintf(resultado, sizeof(resultado),
                     "Tempo esgotado! Sua palavra: (nenhuma). 0 pontos.");
        } else if (repetidas) {
            snprintf(resultado, sizeof(resultado),
                     "Palavras iguais! Sua: \"%s\" | Oponente: \"%s\". Ninguem pontua.",
                     palavra[j], palavra[outro]);
        } else if (valido[j]) {
            snprintf(resultado, sizeof(resultado),
                     "Palavra \"%s\" valida! +1 ponto. [%s enviou: \"%s\"]",
                     palavra[j], p->j[outro].nome,
                     strlen(palavra[outro]) ? palavra[outro] : "(nenhuma)");
        } else {
            snprintf(resultado, sizeof(resultado),
                     "Palavra \"%s\" invalida! 0 pontos. [%s enviou: \"%s\"]",
                     strlen(palavra[j]) ? palavra[j] : "(nenhuma)",
                     p->j[outro].nome,
                     strlen(palavra[outro]) ? palavra[outro] : "(nenhuma)");
        }

        enviar_msg(p->j[j].fd, P_RESULTADO, resultado);
    }

    /*Envia PLACAR atualizado */
    char placar[BUFFER_SIZE];
    snprintf(placar, sizeof(placar), "%s|%d|%s|%d",
             p->j[0].nome, p->j[0].pontos,
             p->j[1].nome, p->j[1].pontos);
    enviar_msg(p->j[0].fd, P_PLACAR, placar);
    enviar_msg(p->j[1].fd, P_PLACAR, placar);

    printf("  [Rodada %d] %s=\"%s\"(%s) | %s=\"%s\"(%s) | Placar: %d x %d\n",
           num_rodada,
           p->j[0].nome, strlen(palavra[0]) ? palavra[0] : "-",
           valido[0] && !repetidas ? "ok" : "x",
           p->j[1].nome, strlen(palavra[1]) ? palavra[1] : "-",
           valido[1] && !repetidas ? "ok" : "x",
           p->j[0].pontos, p->j[1].pontos);
}


static void *executar_partida(void *arg)
{
    Partida *p = (Partida *)arg;
    char buf[BUFFER_SIZE];

    printf("[Partida #%d] Jogadores: %s vs %s\n",
           p->num_partida, p->j[0].nome, p->j[1].nome);

    /* Anuncia início da partida */
    snprintf(buf, sizeof(buf), "Batalha de Palavras! %s vs %s — %d rodadas. Boa sorte!",
             p->j[0].nome, p->j[1].nome, NUM_RODADAS);
    enviar_msg(p->j[0].fd, P_MSG, buf);
    enviar_msg(p->j[1].fd, P_MSG, buf);

    /*Executa NUM_RODADAS rodadas  */
    for (int r = 1; r <= NUM_RODADAS; r++)
        executar_rodada(p, r);

    /* Resultado final */
    char fim[BUFFER_SIZE];
    if (p->j[0].pontos > p->j[1].pontos) {
        snprintf(fim, sizeof(fim), "%s venceu! Placar final: %s %d x %d %s",
                 p->j[0].nome, p->j[0].nome, p->j[0].pontos,
                 p->j[1].pontos, p->j[1].nome);
        printf("[Partida #%d] %s venceu! Placar: %d x %d\n",
               p->num_partida, p->j[0].nome,
               p->j[0].pontos, p->j[1].pontos);
    } else if (p->j[1].pontos > p->j[0].pontos) {
        snprintf(fim, sizeof(fim), "%s venceu! Placar final: %s %d x %d %s",
                 p->j[1].nome, p->j[0].nome, p->j[0].pontos,
                 p->j[1].pontos, p->j[1].nome);
        printf("[Partida #%d] %s venceu! Placar: %d x %d\n",
               p->num_partida, p->j[1].nome,
               p->j[0].pontos, p->j[1].pontos);
    } else {
        snprintf(fim, sizeof(fim), "Empate! Placar final: %s %d x %d %s",
                 p->j[0].nome, p->j[0].pontos,
                 p->j[1].pontos, p->j[1].nome);
        printf("[Partida #%d] Empate! Placar: %d x %d\n",
               p->num_partida, p->j[0].pontos, p->j[1].pontos);
    }

    enviar_msg(p->j[0].fd, P_FIM, fim);
    enviar_msg(p->j[1].fd, P_FIM, fim);

    close(p->j[0].fd);
    close(p->j[1].fd);
    free(p);
    return NULL;
}

int main(int argc, char *argv[])
{
    /* Ignora SIGPIPE — evita crash ao escrever em socket fechado */
    signal(SIGPIPE, SIG_IGN);
    srand((unsigned int)time(NULL));

    int porta = PORTA_PADRAO;
    if (argc == 2) porta = atoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(porta);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind"); close(server_fd); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) == -1) {
        perror("listen"); close(server_fd); exit(EXIT_FAILURE);
    }

    printf("╔══════════════════════════════════════════════╗\n");
    printf("║      BATALHA DE PALAVRAS — Servidor          ║\n");
    printf("║  Porta: %-4d                                 ║\n", porta);
    printf("║  Aguardando jogadores (pares de 2)...        ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);

        int novo_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (novo_fd == -1) { perror("accept"); continue; }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ip, sizeof(ip));
        printf("[+] Jogador conectou: %s:%d (fd=%d)\n",
               ip, ntohs(cli_addr.sin_port), novo_fd);

        /* Solicita o nome */
        enviar_msg(novo_fd, P_NOME, "");

        char nome[NOME_SIZE] = {0};
        int ret = receber_com_timeout(novo_fd, nome, NOME_SIZE, 30);
        if (ret <= 0) {
            printf("[-] Jogador desconectou antes de informar nome\n");
            close(novo_fd);
            continue;
        }

        /* Espera formato NOME|alice — extrai o nome */
        char *nome_val = nome;
        if (strncmp(nome, "NOME|", 5) == 0)
            nome_val = nome + 5;
        if (strlen(nome_val) == 0) {
            close(novo_fd);
            continue;
        }

        printf("[+] Jogador \"%s\" aguardando par\n", nome_val);

        pthread_mutex_lock(&mutex_espera);

        if (fd_esperando == -1) {
            /* Nenhum jogador aguardando: este vai esperar */
            fd_esperando = novo_fd;
            strncpy(nome_esperando, nome_val, NOME_SIZE - 1);
            enviar_msg(novo_fd, P_AGUARDE, "Conectado! Aguardando outro jogador para iniciar...");
            printf("[*] Aguardando mais 1 jogador...\n");
            pthread_mutex_unlock(&mutex_espera);
        } else {
            /* Já há um jogador esperando: forma o par e inicia partida */
            Partida *p = malloc(sizeof(Partida));
            if (!p) { close(novo_fd); pthread_mutex_unlock(&mutex_espera); continue; }

            p->j[0].fd     = fd_esperando;
            p->j[0].pontos = 0;
            strncpy(p->j[0].nome, nome_esperando, NOME_SIZE - 1);

            p->j[1].fd     = novo_fd;
            p->j[1].pontos = 0;
            strncpy(p->j[1].nome, nome_val, NOME_SIZE - 1);

            p->num_partida = ++contador_partidas;

            fd_esperando = -1; /* limpa a fila de espera */
            pthread_mutex_unlock(&mutex_espera);

            pthread_t tid;
            if (pthread_create(&tid, NULL, executar_partida, p) != 0) {
                perror("pthread_create");
                close(p->j[0].fd);
                close(p->j[1].fd);
                free(p);
            } else {
                pthread_detach(tid);
            }
        }
    }

    close(server_fd);
    return 0;
}
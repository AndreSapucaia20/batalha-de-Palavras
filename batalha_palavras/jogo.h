/*
 * jogo.h — Interface da lógica do jogo
 *
 * Funções utilitárias usadas por servidor.c e cliente.c.
 */

#ifndef JOGO_H
#define JOGO_H

#include "protocolo.h"

/*
 * Gera uma letra aleatória (maiúscula) para a rodada.
 * Evita letras difíceis como K, W, X, Y, Z.
 */
char gerar_letra(void);

/*
 * Valida se uma palavra atende às regras da rodada:
 *   - Começa com 'letra' (case insensitive)
 *   - Tem pelo menos MIN_CHARS caracteres
 *   - Contém apenas letras (a-z, A-Z)
 *
 * Retorna 1 se válida, 0 caso contrário.
 */
int validar_palavra(const char *palavra, char letra);

/*
 * Envia uma mensagem no formato "TIPO|corpo\n" para o fd informado.
 * Se corpo for NULL ou vazio, envia "TIPO|\n".
 *
 * Retorna o resultado de send().
 */
int enviar_msg(int fd, const char *tipo, const char *corpo);

/*
 * Recebe uma linha do fd com timeout de 'segundos'.
 *   - Retorna  1  se recebeu dados (buf preenchido, sem \n)
 *   - Retorna  0  se timeout esgotou
 *   - Retorna -1  se erro ou conexão fechada
 */
int receber_com_timeout(int fd, char *buf, int tamanho, int segundos);

#endif /* JOGO_H */
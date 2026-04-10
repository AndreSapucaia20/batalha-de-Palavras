/*
 * protocolo.h — Definições do protocolo de comunicação
 *
 * Compartilhado entre servidor.c e cliente.c.
 * Define portas, constantes do jogo e prefixos das mensagens.
 */

#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#define PORTA_PADRAO   7070
#define MAX_CLIENTES   10
#define BUFFER_SIZE    2048
#define NOME_SIZE      32

#define NUM_RODADAS    5
#define TEMPO_RODADA   10   /* segundos */
#define MIN_CHARS      5    /* tamanho mínimo da palavra */

/* Mensagens: Servidor → Cliente
#define P_MSG        "MSG"        /* MSG|texto                        */
#define P_NOME       "NOME"  
#define P_AGUARDE    "AGUARDE" 
#define P_RODADA     "RODADA" 
#define P_RESULTADO  "RESULTADO"  
#define P_PLACAR     "PLACAR"    
#define P_FIM        "FIM"        

/* Mensagens: Cliente → Servidor 
#define P_PALAVRA    "PALAVRA"    /* PALAVRA|palavra_digitada         */
#define P_TIMEOUT    "TIMEOUT" 

#endif /* PROTOCOLO_H */
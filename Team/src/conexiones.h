#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <string.h>
#include <pthread.h>
#include "entrenador.h"
#include "Team.h"
#include "utils.h"
#include "configTeam.h"
#include "logTeam.h"


typedef struct
{
	uint32_t size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

pthread_t thread;

int crear_conexion(char* ip, char* puerto);

void iniciar_espera_mensajes_Gameboy(void);
void esperar_cliente(int);
void process_request(op_code cod_op, int cliente_fd);
void serve_client(int *socket);
void* serializar_paquete(t_paquete* paquete, int* bytes);

//SUSCRIPCIONES
void* suscribirse_a_cola(int socket_cliente, op_code cola, int* tamanio_paquete);
int enviar_suscripcion_a_cola(op_code cola);

//ENVIAR MENSAJE A BROKER
void* enviar_ACK(int socket_broker, int* tamanio, uint32_t mensaje_id);
void responder_ack(int socket_broker,uint32_t mensaje_id);
void enviar_CatchPokemon_a_broker(op_code codigo_operacion, t_entrenador* entrenador);
void enviar_GetPokemon_a_broker(op_code codigo_operacion, char* pokemon);
void* iniciar_paquete_serializado_CatchPokemon(int* tamanio_paquete,t_entrenador* entrenador);
void* iniciar_paquete_serializado_GetPokemon(int* tamanio_paquete,char* pokemon_pedido);

//CONEXION BROKER
void appeared_pokemon_broker();
void caught_pokemon_broker();
void localized_pokemon_broker();

//RECIBIR MENSAJES GAMEBOY Y BROKER
char* recibir_mensaje(int socket_cliente);
void recibir_AppearedPokemon(int socket_cliente);
void recibir_CaughtPokemon(int socket_cliente);
void recibir_LocalizedPokemon(int socket_cliente);
void recibir_id_correlativo_get(int socket_broker);

//RECIBIR MENSAJES CON LOGS
void recibir_appeared_pokemon_loggeo(void);
void recibir_caught_pokemon_loggeo(int socket_cliente);
void recibir_localized_pokemon_loggeo(int socket_cliente);

t_pokemon* armarPokemon(char* pokemon, int posX, int posY);
t_entrenador* buscar_entrenador_por_id_catch(uint32_t id);

#endif

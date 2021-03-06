#ifndef ENTRENADOR_H_
#define ENTRENADOR_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/config.h>

typedef enum{

	NEW = 0,
	READY = 1,
	BLOCKED = 2,
	EXEC = 3,
	EXIT = 4

} op_estado;

typedef struct
{
	int x;
	int y;

} t_posicion;


typedef struct
{
	char* especie;
	t_posicion posicion;

} t_pokemon;


typedef struct
{
	char ID_entrenador;
	int ciclos_de_cpu_totales;
	int rafaga_anterior;
	op_estado estado;
	t_list* objetivo;
	t_posicion posicion;
	t_list* atrapados;
	t_pokemon* pokemon_a_atrapar;
	pthread_t hilo_entrenador;
	t_list* cola_de_acciones;
	uint32_t ID_catch_pokemon;
	uint32_t pudo_atrapar_pokemon;
	t_posicion posicionIntercambio;

} t_entrenador;

//PREGUNTAR BIEN ESTA PARTE

typedef struct
{
	void(*accion)(void*);
	int ciclo_cpu;

}t_accion;


t_entrenador* armar_entrenador(int indice);
t_posicion obtener_posicion(char* posicion);
t_list* obtener_objetivos(char* objetivos);
t_list* obtener_atrapados(char* atrapados);
t_list* entrenadores_mas_cercanos(t_pokemon* pokemon);
int sacar_distancia(t_posicion pokeposicion,t_posicion entreposicion);
bool puede_atrapar(t_entrenador* entrenador);
void ejecutar_entrenador(t_entrenador* entrenador);
bool termino_con_pokemon(t_entrenador* entrenador, char* pokemon);

void intentar_atrapar_pokemon(t_entrenador* entrenador);
void atrapar_pokemon(t_entrenador* entrenador);
void verificar_estado_entrenador(t_entrenador* entrenador);

//intercambio de pokemones
void preparar_intercambio();
t_entrenador* obtener_entrenador_con_pokemon_del_entrenador1(t_entrenador* entrenador1);
void intercambiar_pokemones(t_entrenador* entrenador1);
bool es_intercambiable_pokemon(t_entrenador* entrenador, char* pokemon);
bool necesita_pokemon(t_entrenador* entrenador, char* pokemon);

void remover_entrenador(t_entrenador* entrenador);
void remover_entrenador_ready(t_entrenador* entrenador);
bool esta_entrenador_en_lista(t_entrenador* entrenador ,t_list* lista);
bool esta_en_exit(t_entrenador* entrenador);

#endif /* ENTRENADOR_H_ */

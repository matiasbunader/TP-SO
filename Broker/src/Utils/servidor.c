#include "servidor.h"

// CONEXION CON CLIENTE

void iniciar_servidor(void)
{
	int socket_servidor;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char* ip = obtener_ip_broker();
    char* puerto =obtener_puerto_broker();

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next){
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1){
            close(socket_servidor);
            continue;
        }
        break;
    }

	if(listen(socket_servidor, SOMAXCONN) == -1){
		close(socket_servidor);
		exit(6);
	}

    freeaddrinfo(servinfo);

    while(1)
    	esperar_cliente(socket_servidor);
}

void esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

	pthread_create(&thread,NULL,(void*)serve_client,&socket_cliente);
	pthread_detach(thread);
}

void serve_client(int* socket_proceso)
{
	log_conexion(*socket_proceso);

	op_code cod_op;
	if(recv(*socket_proceso, &cod_op, sizeof(op_code), MSG_WAITALL) == -1){
		pthread_exit(NULL);
	}

	process_request(cod_op, *socket_proceso);
}

// ATENDER AL CLIENTE

void process_request(op_code cod_op, int socket_cliente) {

	switch (cod_op) {
		case 0:
			atender_suscripcion(socket_cliente);
			//pthread_create(&hilo_suscripcion, NULL, atender_suscripcion, socket_cliente);
			//pthread_detach(hilo_suscripcion);
			enviar_mensaje(socket_cliente, "Suscripto.");
			break;
		case 1:
			log_mensaje_nuevo(cod_op);
			recibir_new_pokemon(socket_cliente);
			//pthread_create(&hilo_newPokemon, NULL, recibir_new_pokemon, socket_cliente);
			//pthread_detach(hilo_newPokemon);
			break;
		case 2:
			log_mensaje_nuevo(cod_op);
			recibir_appeared_pokemon(socket_cliente);
			//pthread_create(&hilo_appearedPokemon, NULL, recibir_appeared_pokemon, socket_cliente);
			//pthread_detach(hilo_appearedPokemon);
			break;
		case 3:
			log_mensaje_nuevo(cod_op);
			recibir_catch_pokemon(socket_cliente);
			//pthread_create(&hilo_catchPokemon, NULL, recibir_catch_pokemon, socket_cliente);
			//pthread_detach(hilo_catchPokemon);
			break;
		case 4:
			log_mensaje_nuevo(cod_op);
			recibir_caught_pokemon(socket_cliente);
			//pthread_create(&hilo_caughtPokemon, NULL, recibir_caught_pokemon, socket_cliente);
			//pthread_detach(hilo_caughtPokemon);
			break;
		case 5:
			log_mensaje_nuevo(cod_op);
			recibir_get_pokemon(socket_cliente);
			//pthread_create(&hilo_getPokemon, NULL, recibir_get_pokemon, socket_cliente);
			//pthread_detach(hilo_getPokemon);
			break;
		case 6:
			log_mensaje_nuevo(cod_op);
			recibir_localized_pokemon(socket_cliente);
			//pthread_create(&hilo_localizedPokemon, NULL, recibir_localized_pokemon, socket_cliente);
			//pthread_detach(hilo_localizedPokemon);
			break;
		default:
			break;
	}

}


// ATENDER SUSCRIPCION

void atender_suscripcion(int socket_cliente)
{
	sem_wait(&SUBS);
	uint32_t tamanio_buffer;
	tamanio_buffer = recibir_tamanio_buffer(socket_cliente);

	proceso* suscriptor = modelar_proceso(socket_cliente);

	suscribirse_a_cola(suscriptor, socket_cliente, tamanio_buffer);
}

proceso* modelar_proceso(int socket){

	proceso* suscriptor = malloc(sizeof(proceso));

	recv(socket, &(suscriptor->id), sizeof(uint32_t), MSG_WAITALL);
	suscriptor->socket_cliente = socket;

	return suscriptor;
}


void suscribirse_a_cola(proceso* suscriptor, int socket, uint32_t tamanio_buffer){

	uint32_t cola_a_suscribirse;
	recv(socket, &cola_a_suscribirse, sizeof(uint32_t), MSG_WAITALL);
	sem_wait(&MUTEX_MEMORIA);
	switch(cola_a_suscribirse){
		case 0:
			break;
		case 1:
			list_add(suscriptores_new_pokemon, suscriptor);
			log_suscripcion_nueva(suscriptor->socket_cliente, suscriptor->id, 1);
			enviar_mensajes_al_nuevo_suscriptor_NP(mensajes_de_cola_new_pokemon, suscriptor->socket_cliente);
			break;
		case 2:
			list_add(suscriptores_appeared_pokemon, suscriptor);
			log_suscripcion_nueva(suscriptor->socket_cliente, suscriptor->id, 2);
			enviar_mensajes_al_nuevo_suscriptor_AP(mensajes_de_cola_appeared_pokemon, suscriptor->socket_cliente);
			break;
		case 3:
			list_add(suscriptores_catch_pokemon, suscriptor);
			log_suscripcion_nueva(suscriptor->socket_cliente, suscriptor->id, 3);
			enviar_mensajes_al_nuevo_suscriptor_CATP(mensajes_de_cola_catch_pokemon, suscriptor->socket_cliente);
			break;
		case 4:
			list_add(suscriptores_caught_pokemon, suscriptor);
			log_suscripcion_nueva(suscriptor->socket_cliente, suscriptor->id, 4);
			enviar_mensajes_al_nuevo_suscriptor_CAUP(mensajes_de_cola_caught_pokemon, suscriptor->socket_cliente);
			break;
		case 5:
			list_add(suscriptores_get_pokemon, suscriptor);
			log_suscripcion_nueva(suscriptor->socket_cliente, suscriptor->id, 5);
			enviar_mensajes_al_nuevo_suscriptor_GP(mensajes_de_cola_get_pokemon, suscriptor->socket_cliente);
			break;
		case 6:
			list_add(suscriptores_localized_pokemon, suscriptor);
			log_suscripcion_nueva(suscriptor->socket_cliente, suscriptor->id, 6);
			enviar_mensajes_al_nuevo_suscriptor_LP(mensajes_de_cola_localized_pokemon, suscriptor->socket_cliente);
			break;
	}
	sem_post(&MUTEX_MEMORIA);

	if(tamanio_buffer == 12){ // Entonces la suscripci??n es del GameBoy

		uint32_t tiempo_de_suscripcion;
		recv(socket, &tiempo_de_suscripcion, sizeof(uint32_t), MSG_WAITALL);

		t_suscripcion* suscripcion = malloc(sizeof(t_suscripcion));
		suscripcion->cola = cola_a_suscribirse;
		suscripcion->socket_cliente = suscriptor->socket_cliente;
		suscripcion->tiempo = tiempo_de_suscripcion;

		pthread_t hilo_recibir;
		pthread_create(&hilo_recibir, NULL , correr_tiempo_suscripcion , suscripcion);
		pthread_join(hilo_recibir, NULL);

		pthread_mutex_lock(&mutex_suscripcion);
	}
	sem_post(&SUBS);

}

void correr_tiempo_suscripcion(t_suscripcion* suscripcion){
	sleep(suscripcion->tiempo);
	proceso* suscriptor;
	int index = -1;

	switch(suscripcion->cola){
		case 1:
			index = encontrar_suscriptor_por_posicion(suscripcion->socket_cliente, suscriptores_new_pokemon);
			suscriptor = list_remove(suscriptores_new_pokemon, index);
			break;
		case 2:
			index = encontrar_suscriptor_por_posicion(suscripcion->socket_cliente, suscriptores_appeared_pokemon);
			suscriptor = list_remove(suscriptores_appeared_pokemon, index);
			break;
		case 3:
			index = encontrar_suscriptor_por_posicion(suscripcion->socket_cliente, suscriptores_catch_pokemon);
			suscriptor = list_remove(suscriptores_catch_pokemon, index);
			break;
		case 4:
			index = encontrar_suscriptor_por_posicion(suscripcion->socket_cliente, suscriptores_caught_pokemon);
			suscriptor = list_remove(suscriptores_caught_pokemon, index);
			break;
		case 5:
			index = encontrar_suscriptor_por_posicion(suscripcion->socket_cliente, suscriptores_get_pokemon);
			suscriptor = list_remove(suscriptores_get_pokemon, index);
			break;
		case 6:
			index = encontrar_suscriptor_por_posicion(suscripcion->socket_cliente, suscriptores_localized_pokemon);
			suscriptor = list_remove(suscriptores_localized_pokemon, index);
			break;
		}

	free(suscriptor);
	free(suscripcion);

	pthread_mutex_unlock(&mutex_suscripcion);

}

int encontrar_suscriptor_por_posicion(int socket_cliente, t_list* lista){
	proceso* suscriptor = NULL;
	for(int i = 0; i<list_size(lista); i++){
		suscriptor = list_get(lista, i);
		if(suscriptor->socket_cliente == socket_cliente){
			return i;
		}
	}
	return -1;
}

//--------------------------------------------------------------------

void enviar_mensajes_al_nuevo_suscriptor_NP(t_list* mensajes_de_dicha_cola, int socket_suscriptor){
	int tamanio_lista = list_size(mensajes_de_dicha_cola);
	t_mensaje_en_cola* mensaje_a_enviar;

	for(int i = 0; i<tamanio_lista; i++){

		mensaje_a_enviar = list_get(mensajes_de_dicha_cola, i);

		sem_wait(&MUTEX_TIMESTAMP);
		timestamp+=10;
		mensaje_a_enviar->ubicacion_mensaje->ultima_referencia = timestamp;
		sem_post(&MUTEX_TIMESTAMP);

		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete->codigo_operacion = mensaje_a_enviar->ubicacion_mensaje->cola;
		paquete->buffer = malloc(sizeof(t_buffer));
		paquete->buffer->size = mensaje_a_enviar->tamanio_buffer + sizeof(uint32_t);

		void* stream = malloc(paquete->buffer->size);
		int offset_stream = 0;
		int offset_memoria = mensaje_a_enviar->ubicacion_mensaje->byte_comienzo_ocupado;

		uint32_t id_mensaje = mensaje_a_enviar->ubicacion_mensaje->id;
		memcpy(stream + offset_stream, &id_mensaje, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);

		uint32_t caracteres;
		memcpy(&caracteres, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &caracteres, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		char* pokemon = malloc(caracteres);
		pokemon = mensaje_a_enviar->pokemon;
		memcpy(stream + offset_stream, pokemon, caracteres);
		offset_stream += caracteres;
		offset_memoria += (caracteres-1);

		uint32_t posx;
		memcpy(&posx, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &posx, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		uint32_t posy;
		memcpy(&posy, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &posy, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		uint32_t cantidad;
		memcpy(&cantidad, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &cantidad, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		paquete->buffer->stream = stream;
		int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
		void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

		if(send(socket_suscriptor,a_enviar,tamanio_paquete,0) == -1){
			completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
			exit(3);
		}

		log_envio_mensaje(socket_suscriptor, 1);

		pthread_t hilo_ack;
		pthread_create(&hilo_ack, NULL, recibir_ack, socket_suscriptor);
		pthread_detach(hilo_ack);

		free(paquete);
		free(paquete->buffer);
		free(stream);
		free(pokemon);
	}
}

void enviar_mensajes_al_nuevo_suscriptor_AP(t_list* mensajes_de_dicha_cola, int socket_suscriptor){
	int tamanio_lista = list_size(mensajes_de_dicha_cola);
	t_mensaje_en_cola* mensaje_a_enviar;

	for(int i = 0; i<tamanio_lista; i++){

		mensaje_a_enviar = list_get(mensajes_de_dicha_cola, i);

		sem_wait(&MUTEX_TIMESTAMP);
		timestamp+=10;
		mensaje_a_enviar->ubicacion_mensaje->ultima_referencia = timestamp; // Aumento el timestamp
		sem_post(&MUTEX_TIMESTAMP);

		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete->codigo_operacion = mensaje_a_enviar->ubicacion_mensaje->cola;
		paquete->buffer = malloc(sizeof(t_buffer));
		paquete->buffer->size = mensaje_a_enviar->tamanio_buffer + sizeof(uint32_t);

		void* stream = malloc(paquete->buffer->size);
		int offset_stream = 0;
		int inicio_mensaje = mensaje_a_enviar->ubicacion_mensaje->byte_comienzo_ocupado;
		int offset_memoria = inicio_mensaje;

		uint32_t id_mensaje = mensaje_a_enviar->ubicacion_mensaje->id;
		memcpy(stream + offset_stream, &id_mensaje, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);

		uint32_t caracteres;
		memcpy(&caracteres, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &caracteres, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		char* pokemon = malloc(caracteres);
		pokemon = mensaje_a_enviar->pokemon;
		memcpy(stream + offset_stream, pokemon, caracteres);
		offset_stream += caracteres;
		offset_memoria += (caracteres-1);

		uint32_t posx;
		memcpy(&posx, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &posx, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		uint32_t posy;
		memcpy(&posy, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &posy, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		paquete->buffer->stream = stream;
		int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
		void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

		if(send(socket_suscriptor,a_enviar,tamanio_paquete,0) == -1){
			completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
			exit(3);
		}

		log_envio_mensaje(socket_suscriptor,2);

		pthread_t hilo_ack;
		pthread_create(&hilo_ack, NULL, recibir_ack, socket_suscriptor);
		pthread_detach(hilo_ack);

		free(paquete);
		free(paquete->buffer);
		free(stream);
		free(pokemon);
	}
}

void enviar_mensajes_al_nuevo_suscriptor_CATP(t_list* mensajes_de_dicha_cola, int socket_suscriptor){
	int tamanio_lista = list_size(mensajes_de_dicha_cola);
	t_mensaje_en_cola* mensaje_a_enviar;

	for(int i = 0; i<tamanio_lista; i++){

		mensaje_a_enviar = list_get(mensajes_de_dicha_cola, i);

		sem_wait(&MUTEX_TIMESTAMP);
		timestamp+=10;
		mensaje_a_enviar->ubicacion_mensaje->ultima_referencia = timestamp; // Aumento el timestamp
		sem_post(&MUTEX_TIMESTAMP);

		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete->codigo_operacion = mensaje_a_enviar->ubicacion_mensaje->cola;
		paquete->buffer = malloc(sizeof(t_buffer));
		paquete->buffer->size = mensaje_a_enviar->tamanio_buffer + sizeof(uint32_t);

		void* stream = malloc(paquete->buffer->size);
		int offset_stream = 0;
		int inicio_mensaje = mensaje_a_enviar->ubicacion_mensaje->byte_comienzo_ocupado;
		int offset_memoria = inicio_mensaje;

		uint32_t id_del_mensaje = mensaje_a_enviar->ubicacion_mensaje->id;
		memcpy(stream + offset_stream, &id_del_mensaje, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);

		uint32_t caracteres;
		memcpy(&caracteres, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &caracteres, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		char* pokemon = malloc(caracteres);
		pokemon = mensaje_a_enviar->pokemon;
		memcpy(stream + offset_stream, pokemon, caracteres);
		offset_stream += caracteres;
		offset_memoria += (caracteres-1);

		uint32_t posx;
		memcpy(&posx, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &posx, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		uint32_t posy;
		memcpy(&posy, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &posy, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		paquete->buffer->stream = stream;

		int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
		void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

		if(send(socket_suscriptor,a_enviar,tamanio_paquete,0) == -1){
			completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
			exit(3);
		}

		log_envio_mensaje(socket_suscriptor,3);

		pthread_t hilo_ack;
		pthread_create(&hilo_ack, NULL, recibir_ack, socket_suscriptor);
		pthread_detach(hilo_ack);

		free(paquete);
		free(paquete->buffer);
		free(stream);
		free(pokemon);
	}
}

void enviar_mensajes_al_nuevo_suscriptor_CAUP(t_list* mensajes_de_dicha_cola, int socket_suscriptor){

	int tamanio_lista = list_size(mensajes_de_dicha_cola);
	t_mensaje_en_cola* mensaje_a_enviar;

	for(int i = 0; i<tamanio_lista; i++){

		mensaje_a_enviar = list_get(mensajes_de_dicha_cola, i);

		sem_wait(&MUTEX_TIMESTAMP);
		timestamp+=10;
		mensaje_a_enviar->ubicacion_mensaje->ultima_referencia = timestamp; // Aumento el timestamp
		sem_post(&MUTEX_TIMESTAMP);

		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete->codigo_operacion = mensaje_a_enviar->ubicacion_mensaje->cola;
		paquete->buffer = malloc(sizeof(t_buffer));
		paquete->buffer->size = mensaje_a_enviar->tamanio_buffer + sizeof(uint32_t);

		void* stream = malloc(paquete->buffer->size);
		int inicio_mensaje = mensaje_a_enviar->ubicacion_mensaje->byte_comienzo_ocupado;
		int tamanio_mensaje = mensaje_a_enviar->ubicacion_mensaje->tamanio_ocupado;
		int desplazamiento = 0;

		uint32_t id_mensaje = mensaje_a_enviar->ubicacion_mensaje->id;
		memcpy(stream + desplazamiento, &id_mensaje, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		uint32_t id_correlativo = mensaje_a_enviar->id_mensaje_correlativo;
		memcpy(stream + desplazamiento, &id_correlativo, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(stream + desplazamiento, (memoria_principal + inicio_mensaje), tamanio_mensaje);

		paquete->buffer->stream = stream; //mensaje_a_enviar->contenido_del_mensaje;

		int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
		void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

		if(send(socket_suscriptor,a_enviar,tamanio_paquete,0) == -1){
			completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
			exit(3);
		}

		log_envio_mensaje(socket_suscriptor,4);

		pthread_t hilo_ack;
		pthread_create(&hilo_ack, NULL, recibir_ack, socket_suscriptor);
		pthread_detach(hilo_ack);

		free(paquete);
		free(paquete->buffer);
		free(stream);
	}
}

void enviar_mensajes_al_nuevo_suscriptor_GP(t_list* mensajes_de_dicha_cola, int socket_suscriptor){
	int tamanio_lista = list_size(mensajes_de_dicha_cola);
	t_mensaje_en_cola* mensaje_a_enviar;

	for(int i = 0; i<tamanio_lista; i++){

		mensaje_a_enviar = list_get(mensajes_de_dicha_cola, i);

		sem_wait(&MUTEX_TIMESTAMP);
		timestamp+=10;
		mensaje_a_enviar->ubicacion_mensaje->ultima_referencia = timestamp; // Aumento el timestamp
		sem_post(&MUTEX_TIMESTAMP);

		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete->codigo_operacion = mensaje_a_enviar->ubicacion_mensaje->cola;
		paquete->buffer = malloc(sizeof(t_buffer));
		paquete->buffer->size = mensaje_a_enviar->tamanio_buffer + sizeof(uint32_t);

		void* stream = malloc(paquete->buffer->size);
		int offset_stream = 0;
		int inicio_mensaje = mensaje_a_enviar->ubicacion_mensaje->byte_comienzo_ocupado;
		int offset_memoria = inicio_mensaje;

		uint32_t id_del_mensaje = mensaje_a_enviar->ubicacion_mensaje->id;
		memcpy(stream + offset_stream, &id_del_mensaje, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);

		uint32_t caracteres;
		memcpy(&caracteres, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &caracteres, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		char* pokemon = malloc(caracteres);
		pokemon = mensaje_a_enviar->pokemon;
		memcpy(stream + offset_stream, pokemon, caracteres);

		paquete->buffer->stream = stream;

		int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
		void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

		if(send(socket_suscriptor,a_enviar,tamanio_paquete,0) == -1){
			completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
			exit(3);
		}

		log_envio_mensaje(socket_suscriptor,5);

		pthread_t hilo_ack;
		pthread_create(&hilo_ack, NULL, recibir_ack, socket_suscriptor);
		pthread_detach(hilo_ack);
/*
		free(paquete);
		free(paquete->buffer);
		free(stream);
		free(pokemon);
*/
	}
}

void enviar_mensajes_al_nuevo_suscriptor_LP(t_list* mensajes_de_dicha_cola, int socket_suscriptor){
	int tamanio_lista = list_size(mensajes_de_dicha_cola);
	t_mensaje_en_cola* mensaje_a_enviar;

	for(int i = 0; i<tamanio_lista; i++){

		mensaje_a_enviar = list_get(mensajes_de_dicha_cola, i);

		sem_wait(&MUTEX_TIMESTAMP);
		timestamp+=10;
		mensaje_a_enviar->ubicacion_mensaje->ultima_referencia = timestamp; // Aumento el timestamp
		sem_post(&MUTEX_TIMESTAMP);

		t_paquete* paquete = malloc(sizeof(t_paquete));
		paquete->codigo_operacion = mensaje_a_enviar->ubicacion_mensaje->cola;
		paquete->buffer = malloc(sizeof(t_buffer));
		paquete->buffer->size = mensaje_a_enviar->tamanio_buffer + sizeof(uint32_t);

		void* stream = malloc(paquete->buffer->size);
		int offset_stream = 0;
		int inicio_mensaje = mensaje_a_enviar->ubicacion_mensaje->byte_comienzo_ocupado;
		int offset_memoria = inicio_mensaje;

		uint32_t id_del_mensaje = mensaje_a_enviar->ubicacion_mensaje->id;
		memcpy(stream + offset_stream, &id_del_mensaje, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);

		uint32_t id_mensaje_correlativo = mensaje_a_enviar->id_mensaje_correlativo;
		memcpy(stream + offset_stream, &id_mensaje_correlativo, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);

		uint32_t caracteres;
		memcpy(&caracteres, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &caracteres, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		char* pokemon = malloc(caracteres);
		pokemon = mensaje_a_enviar->pokemon;
		memcpy(stream + offset_stream, pokemon, caracteres);
		offset_stream += caracteres;
		offset_memoria += (caracteres-1);

		uint32_t cantidad_de_posiciones;
		memcpy(&cantidad_de_posiciones, memoria_principal + offset_memoria, sizeof(uint32_t));
		memcpy(stream + offset_stream, &cantidad_de_posiciones, sizeof(uint32_t));
		offset_stream += sizeof(uint32_t);
		offset_memoria += sizeof(uint32_t);

		int tam_posxposy = cantidad_de_posiciones*8;
		void* posxposy = malloc(tam_posxposy);
		int desp_posxposy = 0;

		for(int i=0; i<cantidad_de_posiciones; i++){

			uint32_t posX;
			memcpy(&posX, memoria_principal + offset_memoria, sizeof(uint32_t));

				memcpy(posxposy + desp_posxposy, &posX, sizeof(uint32_t));
				desp_posxposy += sizeof(uint32_t);

			uint32_t posY;
			memcpy(&posY, memoria_principal + offset_memoria, sizeof(uint32_t));

				memcpy(posxposy + desp_posxposy, &posY, sizeof(uint32_t));
				desp_posxposy += sizeof(uint32_t);
		}

		memcpy(stream + offset_stream, posxposy, tam_posxposy);

		paquete->buffer->stream = stream;

		int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
		void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

		if(send(socket_suscriptor,a_enviar,tamanio_paquete,0) == -1){
			completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
			exit(3);
		}

		log_envio_mensaje(socket_suscriptor,6);

		pthread_t hilo_ack;
		pthread_create(&hilo_ack, NULL, recibir_ack, socket_suscriptor);
		pthread_detach(hilo_ack);

		free(paquete);
		free(paquete->buffer);
		free(stream);
		free(pokemon);
	}
}

// RECEPCION, ALMACENAMIENTO EN MEMORIA Y REENVIO DE MENSAJES

void recibir_new_pokemon(int socket_cliente)
{
	sem_wait(&MUTEX_NEW);
	uint32_t tamanio_buffer;
	recv(socket_cliente, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);

	uint32_t caracteresPokemon;
	recv(socket_cliente, &caracteresPokemon, sizeof(uint32_t), MSG_WAITALL);

	char* pokemon = (char*)malloc(caracteresPokemon);
	recv(socket_cliente, pokemon, caracteresPokemon, MSG_WAITALL);

	uint32_t posX;
	recv(socket_cliente, &posX, sizeof(uint32_t), MSG_WAITALL);

	uint32_t posY;
	recv(socket_cliente, &posY, sizeof(uint32_t), MSG_WAITALL);

	uint32_t cantidad;
	recv(socket_cliente, &cantidad, sizeof(uint32_t), MSG_WAITALL);

	// Creacion de bloque a guardar en memoria

	int buffer_sin_barra_n = tamanio_buffer - 1;

	void* bloque_a_agregar_en_memoria = malloc(buffer_sin_barra_n); // no cuenta el \n del nombre del Pokemon
	int desplazamiento = 0;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, pokemon, caracteresPokemon-1);
	desplazamiento += caracteresPokemon-1;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &posX, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &posY, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &cantidad, sizeof(uint32_t));

	t_mensaje_guardado* mensaje_nuevo;
	mensaje_nuevo = guardar_mensaje_en_memoria(bloque_a_agregar_en_memoria, buffer_sin_barra_n, 1);

	// Enviar id

	enviar_id_mensaje(mensaje_nuevo->id, socket_cliente);

	// Prepararacion y envio de mensaje a suscriptores ---> Una vez que chequeo que se manda, hacerlo con todos

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 1;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = tamanio_buffer + sizeof(uint32_t);

	void* bloque_con_barra_n_e_id = malloc(paquete->buffer->size);
	int desplazamiento_con_barra_n = 0;

	uint32_t id_del_mensaje = mensaje_nuevo->id;
	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &id_del_mensaje, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, pokemon, caracteresPokemon);
	desplazamiento_con_barra_n += caracteresPokemon;

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &posX, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &posY, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &cantidad, sizeof(uint32_t));

	paquete->buffer->stream = bloque_con_barra_n_e_id;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

	reenviar_mensaje_a_suscriptores(a_enviar, tamanio_paquete, suscriptores_new_pokemon, 1);

	// Guardo informaci??n del mensaje

	uint32_t null = -1;
	guardar_mensaje_en_cola(mensajes_de_cola_new_pokemon, mensaje_nuevo, tamanio_buffer, null, pokemon, suscriptores_new_pokemon);

	sem_post(&MUTEX_NEW);

//	free(bloque_a_agregar_en_memoria);
	free(paquete);
	free(paquete->buffer);
}

void recibir_appeared_pokemon(int socket_cliente){
	sem_wait(&MUTEX_APPEARED);

	uint32_t tamanio_buffer;
	recv(socket_cliente, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);

	uint32_t caracteresPokemon;
	recv(socket_cliente, &caracteresPokemon, sizeof(uint32_t), MSG_WAITALL);

	char* pokemon = (char*)malloc(caracteresPokemon);
	recv(socket_cliente, pokemon, caracteresPokemon, MSG_WAITALL);

	uint32_t posX;
	recv(socket_cliente, &posX, sizeof(uint32_t), MSG_WAITALL);

	uint32_t posY;
	recv(socket_cliente, &posY, sizeof(uint32_t), MSG_WAITALL);

	uint32_t id_mensaje_correlativo;
	recv(socket_cliente, &id_mensaje_correlativo, sizeof(uint32_t), MSG_WAITALL);

	// Creacion de bloque a guardar en memoria

	uint32_t tamanio_buffer_sin_barra_n_ni_id = tamanio_buffer-sizeof(uint32_t)-1;
	void* bloque_a_agregar_en_memoria = malloc(tamanio_buffer_sin_barra_n_ni_id); // no cuenta el \n del nombre del Pokemon
	int desplazamiento = 0;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, pokemon, caracteresPokemon-1);
	desplazamiento += caracteresPokemon-1;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &posX, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &posY, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	t_mensaje_guardado* mensaje_nuevo;
	mensaje_nuevo = guardar_mensaje_en_memoria(bloque_a_agregar_en_memoria, tamanio_buffer_sin_barra_n_ni_id, 2);

	// Enviar id

	enviar_id_mensaje(mensaje_nuevo->id, socket_cliente);

	// Prepararacion y envio de mensaje a suscriptores

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 2;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = tamanio_buffer;

	void* bloque_con_barra_n = malloc(paquete->buffer->size);
	int desplazamiento_con_barra_n = 0;

	uint32_t id_del_mensaje = mensaje_nuevo->id;
	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &id_del_mensaje, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, pokemon, caracteresPokemon);
	desplazamiento_con_barra_n += caracteresPokemon;

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &posX, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &posY, sizeof(uint32_t));

	paquete->buffer->stream = bloque_con_barra_n;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, tamanio_paquete);
	reenviar_mensaje_a_suscriptores(a_enviar, tamanio_paquete, suscriptores_appeared_pokemon, 2);

	// Guardo informaci??n del mensaje

	uint32_t tamanio_buffer_sin_id = tamanio_buffer_sin_barra_n_ni_id + 1;
	uint32_t null = -1;
	guardar_mensaje_en_cola(mensajes_de_cola_appeared_pokemon, mensaje_nuevo, tamanio_buffer_sin_id, null, pokemon, suscriptores_appeared_pokemon);

	sem_post(&MUTEX_APPEARED);

//	free(bloque_a_agregar_en_memoria);
	free(paquete->buffer);
	free(paquete);

}

void recibir_catch_pokemon(int socket_cliente){
	sem_wait(&MUTEX_CATCH);

	uint32_t tamanio_buffer;
	recv(socket_cliente, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);

	uint32_t caracteresPokemon;
	recv(socket_cliente, &caracteresPokemon, sizeof(uint32_t), MSG_WAITALL);

	char* pokemon = (char*)malloc(caracteresPokemon);
	recv(socket_cliente, pokemon, caracteresPokemon, MSG_WAITALL);

	uint32_t posX;
	recv(socket_cliente, &posX, sizeof(uint32_t), MSG_WAITALL);

	uint32_t posY;
	recv(socket_cliente, &posY, sizeof(uint32_t), MSG_WAITALL);

	// Creacion de bloque a guardar en memoria

	int buffer_sin_barra_n = tamanio_buffer - 1;
	void* bloque_a_agregar_en_memoria = malloc(buffer_sin_barra_n); // no cuenta el \n del nombre del Pokemon
	int desplazamiento = 0;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, pokemon, caracteresPokemon-1);
	desplazamiento += caracteresPokemon-1;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &posX, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &posY, sizeof(uint32_t));

	t_mensaje_guardado* mensaje_nuevo;
	mensaje_nuevo = guardar_mensaje_en_memoria(bloque_a_agregar_en_memoria, buffer_sin_barra_n, 3);

	// Enviar id

	enviar_id_mensaje(mensaje_nuevo->id, socket_cliente);

	// Prepararacion y envio de mensaje a suscriptores ---> Una vez que chequeo que se manda, hacerlo con todos

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 3;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = tamanio_buffer + sizeof(uint32_t);

	void* bloque_con_barra_n_e_id = malloc(paquete->buffer->size);
	int desplazamiento_con_barra_n = 0;

	uint32_t id_del_mensaje = mensaje_nuevo->id;
	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &id_del_mensaje, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, pokemon, caracteresPokemon);
	desplazamiento_con_barra_n += caracteresPokemon;

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &posX, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n_e_id + desplazamiento_con_barra_n, &posY, sizeof(uint32_t));

	paquete->buffer->stream = bloque_con_barra_n_e_id;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, tamanio_paquete);
	reenviar_mensaje_a_suscriptores(a_enviar, tamanio_paquete, suscriptores_catch_pokemon, 3);

	// Guardo informaci??n del mensaje
	uint32_t null = -1;
	guardar_mensaje_en_cola(mensajes_de_cola_catch_pokemon, mensaje_nuevo, tamanio_buffer, null, pokemon, suscriptores_catch_pokemon);

	sem_post(&MUTEX_CATCH);

//	free(bloque_a_agregar_en_memoria);
	free(paquete->buffer);
	free(paquete);
}

void recibir_caught_pokemon(int socket_cliente){
	sem_wait(&MUTEX_CAUGHT);
	uint32_t tamanio_buffer;
	recv(socket_cliente, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);

	uint32_t id_mensaje_correlativo;
	recv(socket_cliente, &id_mensaje_correlativo, sizeof(uint32_t), MSG_WAITALL);

	uint32_t se_pudo_atrapar;
	recv(socket_cliente, &se_pudo_atrapar, sizeof(uint32_t), MSG_WAITALL);

	// Creacion de bloque a guardar en memoria

	int desplazamiento = 0;

	uint32_t tamanio_buffer_sin_id_mensaje = tamanio_buffer - sizeof(uint32_t);
	void* bloque_a_agregar_en_memoria = malloc(tamanio_buffer_sin_id_mensaje); // no cuenta el \n del nombre del Pokemon

	memcpy(bloque_a_agregar_en_memoria, &se_pudo_atrapar, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	t_mensaje_guardado* mensaje_nuevo;
	mensaje_nuevo = guardar_mensaje_en_memoria(bloque_a_agregar_en_memoria, tamanio_buffer_sin_id_mensaje, 4);

	// Enviar id

	enviar_id_mensaje(mensaje_nuevo->id, socket_cliente);

	// Prepararacion y envio de mensaje a suscriptores

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 4;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = tamanio_buffer + sizeof(uint32_t);

	void* bloque_con_id = malloc(paquete->buffer->size);
	int desplazamiento_con_id = 0;

	uint32_t id_del_mensaje = mensaje_nuevo->id;
	memcpy(bloque_con_id + desplazamiento_con_id, &id_del_mensaje, sizeof(uint32_t));
	desplazamiento_con_id += sizeof(uint32_t);

	memcpy(bloque_con_id + desplazamiento_con_id, &id_mensaje_correlativo, sizeof(uint32_t));
	desplazamiento_con_id += sizeof(uint32_t);

	memcpy(bloque_con_id + desplazamiento_con_id, bloque_a_agregar_en_memoria, tamanio_buffer_sin_id_mensaje);

	paquete->buffer->stream = bloque_con_id;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, tamanio_paquete);
	reenviar_mensaje_a_suscriptores(a_enviar, tamanio_paquete, suscriptores_caught_pokemon, 4);

	// Guardo informaci??n del mensaje

	guardar_mensaje_en_cola(mensajes_de_cola_caught_pokemon, mensaje_nuevo, tamanio_buffer, id_mensaje_correlativo, NULL, suscriptores_caught_pokemon);

	sem_post(&MUTEX_CAUGHT);

//	free(bloque_a_agregar_en_memoria);
	free(paquete->buffer);
	free(paquete);
}

void recibir_get_pokemon(int socket_cliente){
	sem_wait(&MUTEX_GET);

	uint32_t tamanio_buffer;
	recv(socket_cliente, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);

	uint32_t caracteresPokemon;
	recv(socket_cliente, &caracteresPokemon, sizeof(uint32_t), MSG_WAITALL);

	char* pokemon = malloc(caracteresPokemon);
	recv(socket_cliente, pokemon, caracteresPokemon, MSG_WAITALL);

	// Creacion de bloque a guardar en memoria

	int buffer_sin_barra_n = tamanio_buffer - 1;
	void* bloque_a_agregar_en_memoria = malloc(buffer_sin_barra_n);
	int desplazamiento = 0;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, pokemon, caracteresPokemon-1);

	t_mensaje_guardado* mensaje_nuevo;
	mensaje_nuevo = guardar_mensaje_en_memoria(bloque_a_agregar_en_memoria, buffer_sin_barra_n, 5);

	// Enviar id

	enviar_id_mensaje(mensaje_nuevo->id, socket_cliente);

	// Prepararacion y envio de mensaje a suscriptores

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 5;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = tamanio_buffer + sizeof(uint32_t);

	void* bloque_con_barra_n = malloc(paquete->buffer->size);
	int desplazamiento_con_barra_n = 0;

	uint32_t id_del_mensaje = mensaje_nuevo->id;
	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &id_del_mensaje, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, pokemon, caracteresPokemon);

	paquete->buffer->stream = bloque_con_barra_n;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, tamanio_paquete);
	reenviar_mensaje_a_suscriptores(a_enviar, tamanio_paquete, suscriptores_get_pokemon, 5);

	// Guardo informaci??n del mensaje
	uint32_t null = -1;
	guardar_mensaje_en_cola(mensajes_de_cola_get_pokemon, mensaje_nuevo, tamanio_buffer, null, pokemon, suscriptores_get_pokemon);

	sem_post(&MUTEX_GET);

	//free(bloque_a_agregar_en_memoria);
	//free(paquete->buffer);
	//free(paquete);
}

void recibir_localized_pokemon(int socket_cliente){
	sem_wait(&MUTEX_LOCALIZED);

	uint32_t tamanio_buffer;
	recv(socket_cliente, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);

	uint32_t id_mensaje_correlativo;
	recv(socket_cliente, &id_mensaje_correlativo, sizeof(uint32_t), MSG_WAITALL);

	uint32_t caracteresPokemon;
	recv(socket_cliente, &caracteresPokemon, sizeof(uint32_t), MSG_WAITALL);

	char* pokemon = (char*)malloc(caracteresPokemon);
	recv(socket_cliente, pokemon, caracteresPokemon, MSG_WAITALL);

	uint32_t cantidad_de_posiciones;
	recv(socket_cliente, &cantidad_de_posiciones, sizeof(uint32_t), MSG_WAITALL);

	uint32_t tam_posxposy = cantidad_de_posiciones*8;
	void* posxposy = malloc(tam_posxposy);
	int desp_posxposy = 0;

	for(int i = 0; i < cantidad_de_posiciones; i ++){

		uint32_t posX;
		recv(socket_cliente, &posX, sizeof(uint32_t), MSG_WAITALL);

			memcpy(posxposy + desp_posxposy, &posX, sizeof(uint32_t));
			desp_posxposy += sizeof(uint32_t);

		uint32_t posY;
		recv(socket_cliente, &posY, sizeof(uint32_t), MSG_WAITALL);

			memcpy(posxposy + desp_posxposy, &posY, sizeof(uint32_t));
			desp_posxposy += sizeof(uint32_t);
	}

	// Creacion de bloque a guardar en memoria

	int buffer_sin_barra_n_ni_id = tamanio_buffer - 1 - sizeof(uint32_t);
	void* bloque_a_agregar_en_memoria = malloc(buffer_sin_barra_n_ni_id);
	int desplazamiento = 0;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, pokemon, caracteresPokemon-1);
	desplazamiento += caracteresPokemon-1;

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, &cantidad_de_posiciones, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(bloque_a_agregar_en_memoria + desplazamiento, posxposy, tam_posxposy);

	t_mensaje_guardado* mensaje_nuevo;
	mensaje_nuevo = guardar_mensaje_en_memoria(bloque_a_agregar_en_memoria, buffer_sin_barra_n_ni_id, 6);

	// Enviar id

	enviar_id_mensaje(mensaje_nuevo->id, socket_cliente);

	// Prepararacion y envio de mensaje a suscriptores ---> Una vez que chequeo que se manda, hacerlo con todos

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 6;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = tamanio_buffer + sizeof(uint32_t);

	void* bloque_con_barra_n = malloc(paquete->buffer->size);
	int desplazamiento_con_barra_n = 0;

	uint32_t id_del_mensaje = mensaje_nuevo->id;
	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &id_del_mensaje, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &id_mensaje_correlativo, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &caracteresPokemon, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, pokemon, caracteresPokemon);
	desplazamiento_con_barra_n += caracteresPokemon;

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, &cantidad_de_posiciones, sizeof(uint32_t));
	desplazamiento_con_barra_n += sizeof(uint32_t);

	memcpy(bloque_con_barra_n + desplazamiento_con_barra_n, posxposy, tam_posxposy);

	paquete->buffer->stream = bloque_con_barra_n;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

	reenviar_mensaje_a_suscriptores(a_enviar, tamanio_paquete, suscriptores_localized_pokemon, 6);

	// Guardo informaci??n del mensaje

	guardar_mensaje_en_cola(mensajes_de_cola_localized_pokemon, mensaje_nuevo, tamanio_buffer, id_mensaje_correlativo, pokemon, suscriptores_localized_pokemon);

	sem_post(&MUTEX_LOCALIZED);

	//free(bloque_a_agregar_en_memoria);
	//free(posxposy);
	free(paquete);
	free(paquete->buffer);
}

void guardar_mensaje_en_cola(t_list* lista_mensajes, t_mensaje_guardado* mensaje_en_memoria, uint32_t tamanio_buffer, uint32_t id_mensaje_correlativo, char* pokemon, t_list* lista_de_suscriptores){
	sem_post(&GUARDAR);
	t_mensaje_en_cola* nuevo_mensaje = malloc(sizeof(t_mensaje_en_cola));
	nuevo_mensaje->suscriptores_que_no_enviaron_ack = lista_de_suscriptores;
	nuevo_mensaje->tamanio_buffer = tamanio_buffer;
	nuevo_mensaje->ubicacion_mensaje = mensaje_en_memoria;
	nuevo_mensaje->id_mensaje_correlativo = id_mensaje_correlativo;
	nuevo_mensaje->pokemon = pokemon;

	list_add(lista_mensajes, nuevo_mensaje);

	sem_wait(&LISTA_GENERAL);
	list_add(lista_de_todos_los_mensajes, nuevo_mensaje);
	sem_post(&LISTA_GENERAL);

	sem_post(&GUARDAR);
}

void reenviar_mensaje_a_suscriptores(void* a_enviar, int tamanio_paquete, t_list* suscriptores, int cola){
	sem_wait(&REENVIO);
	int tamanio_lista_de_suscriptores = list_size(suscriptores);

	for(int i = 0; i < tamanio_lista_de_suscriptores; i++){

		proceso* suscriptor = list_get(suscriptores, i);
		int socket_suscriptor = suscriptor->socket_cliente;

		if(send(socket_suscriptor,a_enviar,tamanio_paquete,0) == -1){
			completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
			exit(3);
		}

		log_envio_mensaje(socket_suscriptor,cola);

		pthread_t* hilo_ack;
		pthread_create(&hilo_ack, NULL, recibir_ack, socket_suscriptor);
		pthread_detach(hilo_ack);
	}
	sem_post(&REENVIO);
}

void recibir_ack(int socket_cliente){
	op_code cod_op;
	recv(socket_cliente, &cod_op, sizeof(op_code), MSG_WAITALL);

	uint32_t tamanio_buffer;
	recv(socket_cliente, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);

	uint32_t caracteres_mensaje;
	recv(socket_cliente, &caracteres_mensaje, sizeof(uint32_t), MSG_WAITALL);

	char* ack = malloc(caracteres_mensaje);
	recv(socket_cliente, ack, caracteres_mensaje, MSG_WAITALL);

	uint32_t mensaje_id_recibido;
	recv(socket_cliente, &mensaje_id_recibido, sizeof(uint32_t), MSG_WAITALL);

	uint32_t proceso_id;
	recv(socket_cliente, &proceso_id, sizeof(uint32_t), MSG_WAITALL);

	log_confirmacion(proceso_id, mensaje_id_recibido);

	//eliminar_suscriptor_que_ya_ack(mensaje_id_recibido, proceso_id);
}

void eliminar_suscriptor_que_ya_ack(uint32_t mensaje_id_recibido, uint32_t proceso_id){
	t_mensaje_en_cola* mensaje_a_leer;
	for(int i=0; i<(list_size(lista_de_todos_los_mensajes)); i++){
		mensaje_a_leer = list_get(lista_de_todos_los_mensajes, i);

		if(mensaje_a_leer->ubicacion_mensaje->id == mensaje_id_recibido){
			op_code cola_a_la_que_pertenece = mensaje_a_leer->ubicacion_mensaje->cola;
			t_list* lista_correspondiente;

			switch(cola_a_la_que_pertenece){
			case 1:
				lista_correspondiente = mensajes_de_cola_new_pokemon;
				break;
			case 2:
				lista_correspondiente = mensajes_de_cola_appeared_pokemon;
				break;
			case 3:
				lista_correspondiente = mensajes_de_cola_catch_pokemon;
				break;
			case 4:
				lista_correspondiente = mensajes_de_cola_caught_pokemon;
				break;
			case 5:
				lista_correspondiente = mensajes_de_cola_get_pokemon;
				break;
			case 6:
				lista_correspondiente = mensajes_de_cola_localized_pokemon;
				break;
			}

			for(int j=0; j<(list_size(lista_correspondiente)); j++){
				mensaje_a_leer = list_get(lista_correspondiente, j);

				if(mensaje_a_leer->ubicacion_mensaje->id == mensaje_id_recibido){
					t_list* suscriptores_que_no_ack = mensaje_a_leer->suscriptores_que_no_enviaron_ack;

					for(int k=0; k<(list_size(suscriptores_que_no_ack)); k++){
						proceso* suscriptor_ack = list_get(suscriptores_que_no_ack, k);

						if(suscriptor_ack->id == proceso_id){
							suscriptor_ack = list_remove(suscriptores_que_no_ack, k);
							break;
						}
					}
					break;
				}
			}
			break;
		}
	}
}

// AUXILIAR

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void* a_enviar = malloc(bytes);
	int offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));
	offset += sizeof(op_code);

	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset +=sizeof(uint32_t);

	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

	return a_enviar;
}

int recibir_tamanio_buffer(int socket){
	uint32_t tamanio_buffer;
	recv(socket, &tamanio_buffer, sizeof(uint32_t), MSG_WAITALL);
	return tamanio_buffer;
}

// ENVIAR MENSAJE

void enviar_mensaje(int socket_cliente, char* mensaje){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 7;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->stream = mensaje;
	paquete->buffer->size = strlen(mensaje) + 1;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete,tamanio_paquete);

	send(socket_cliente,a_enviar,tamanio_paquete,0);

	free(paquete);
	free(paquete->buffer);
}

void enviar_id_mensaje(uint32_t id_del_mensaje, int socket_cliente){

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = 7;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(uint32_t);

	void* stream = malloc(paquete->buffer->size);
	memcpy(stream, &id_del_mensaje, sizeof(uint32_t));

	paquete->buffer->stream = stream;

	int tamanio_paquete = (paquete->buffer->size)+sizeof(op_code)+sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

	if(send(socket_cliente, a_enviar, tamanio_paquete, 0) == -1){
		completar_logger("Error en enviar por el socket","BROKER", LOG_LEVEL_INFO);
		exit(3);
	}

	free(paquete->buffer);
	free(paquete);
}

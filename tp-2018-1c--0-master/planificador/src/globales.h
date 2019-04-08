/*
 * globales.h
 *
 *  Created on: 24 may. 2018
 *      Author: utnso
 */

#ifndef GLOBALES_H_
#define GLOBALES_H_

#include <pthread.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <biblioteca/estructuras.h>
#include <biblioteca/paquetes.h>

typedef struct {
	int socketESI;
	int tEnEspera;
	double estAnterior;
	double realAnterior;
	char* nombreESI;
} t_infoListos;

typedef struct {
	char* idESI;
	t_infoListos* data;
}t_infoBloqueo;

pthread_t hiloServidor;
pthread_t hiloAlgoritmos;
pthread_t hiloCoordinador;

pthread_mutex_t mutexConsola;
pthread_mutex_t mutexBloqueo;
pthread_mutex_t mutexListo;
pthread_mutex_t modificacion;
pthread_mutex_t mutexLog;
pthread_mutex_t mutexClavesTomadas;
pthread_mutex_t mutexInstruccionConsola;
sem_t ESIentrada;
sem_t continua;

t_dictionary* g_listos;
t_dictionary* g_bloq;
t_dictionary* g_clavesTomadas;

t_log* g_logger;
t_config* g_con;

int g_socketCoordinador;
int g_huboError;
int g_keyMaxima;
int g_instruccionConsola;
int g_huboModificacion;
char* g_algoritmo;
double g_alfa;
double g_est;

t_infoListos* g_enEjecucion;

extern void liberarTodo(void);
extern char* liberarESI(char* key);
extern void desbloquearESI(char* clave);
extern void liberarClaves(char* clave);

#endif /* GLOBALES_H_ */

#ifndef COORDINADOR_H_
#define COORDINADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tablasAdministrativas.h"
#include <commons/config.h>
#include <commons/log.h>
#include <biblioteca/sockets.h>
#include <biblioteca/paquetes.h>
#include <biblioteca/estructuras.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <signal.h>

#define PATH_CONFIG "/home/utnso/workspace/tp-2018-1c--0/configuraciones/coordinador.cfg"


/*---------------------Estructuras-------------------------*/

typedef struct{
	char* puertoConexion;
	char* algoritmoDist;
	int cantidadEntradas;
	int tamanioEntradas;
	int retardo;
	int logDebug;
}t_configuraciones;


/*------------------------Globales-------------------------*/

t_config* g_config;
t_log* g_logger;
t_log* g_loggerDebug;
t_configuraciones* g_configuracion;
t_list* g_tablaDeInstancias;
t_list* g_diccionarioConexiones;
t_list* g_diccionarioClaves;
sem_t g_mutexLog;
sem_t g_mutex_respuesta_set;
sem_t g_mutex_respuesta_store;
sem_t g_peticion;
bool g_respuesta;
int instanciasCompactadas;


/*------------------------FUNCIONES-------------------------*/
void signal_handler(int signum);
int iniciarServidor(char* puerto);
void* procesarPeticion(void* cliente_fd);
void 	procesarPaquete(t_paquete* unPaquete,int socketCliente);
void procesarHandshake(t_paquete* unPaquete,int socketCliente);
void procesarSET(t_paquete* unPaquete,int socketCliente);
void procesarGET(t_paquete* unPaquete,int socketCliente);
void procesarSTORE(t_paquete* unPaquete,int socketCliente);
void procesarNombreESI(t_paquete* unPaquete,int socketCliente);
void procesarNombreInstancia(t_paquete* unPaquete,int socketCliente);
void procesarRespuesta(t_paquete* unPaquete,int socketCliente);
void* procesarClienteDesconectado(int cliente_fd);
void procesarClaveEliminada(t_paquete* unPaquete, int cliente_fd);
void procesarCompactar(t_paquete* paquete, int cliente_fd);
void procesarStatus(t_paquete* paquete,int cliente_fd);
void procesarValor(t_paquete* paquete, int cliente_fd);
void compactarTodasLasInstancias(t_list* tablaDeInstancias, t_list* conexiones);
void logSeguro(char* logLevel,sem_t a,char* format,...);
void armarConfigCoordinador(t_configuraciones *configuracion, t_config *config);
t_instancia* PlanificarInstancia(char* algoritmoDePlanificacion,
																	char* Clave,
	 																t_list* tablaDeInstancias);

#endif /* COORDINADOR_H_ */

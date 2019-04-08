#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include "consola.h"
#include "algoritmos.h"
#include "globales.h"
#include <signal.h>

#define RUTA_CONFIGURACION_PLANIF "/home/utnso/workspace/tp-2018-1c--0/configuraciones/planificador.cfg"

#include <biblioteca/sockets.h>
#include <biblioteca/paquetes.h>
#include <biblioteca/estructuras.h>

void procesarPaquete(t_paquete*, int*);
void recibirHandshakePlanif(t_paquete* unPaquete, int* socketCliente);
void planificar(char* algoritmo);
void iniciarServidor(void* puerto);
void procesarPaqueteESIs(t_paquete* unPaquete, int* socketCliente);
void procesarPaqueteCoordinador(t_paquete* unPaquete, int* socketCliente);
void asignarBloquedas(char** codigos);
int condicionDeTomada(char* nodo);
void claveEstaTomada(char* key, t_list* value);
void atenderCoordinador(void* arg);
void atenderCtrlC(void);
int esiTieneClave(void);

#endif /* PLANIFICADOR_H_ */

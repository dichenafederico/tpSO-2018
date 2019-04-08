/*
 * algoritmos.h
 *
 *  Created on: 20 abr. 2018
 *      Author: utnso
 */

#ifndef ALGORITMOS_H_
#define ALGORITMOS_H_

#include <time.h>
#include <commons/string.h>
#include <stdlib.h>
#include <biblioteca/sockets.h>
#include <commons/collections/list.h>

#include "globales.h"

int g_termino;
int g_bloqueo;
int g_tEjecucion;
int g_claveTomada;

int g_socketEnEjecucion;
char* g_claveGET;
char* g_idESIactual;
char* g_nombreESIactual;

extern void planificarSinDesalojo(char*);
extern void planificarConDesalojo(void);


#endif /* ALGORITMOS_H_ */

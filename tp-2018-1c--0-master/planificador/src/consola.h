#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include <biblioteca/estructuras.h>
#include "globales.h"
#include "algoritmos.h"

/*------------------------------Globales-----------------------------*/
char* g_nombreESI;
char* g_clave;
char* g_idESI;
char* g_elemento;
bool g_bool;
bool g_existenciaClave;
int **g_matrizAsignacion;
int **g_matrizEspera;

t_dictionary* g_clavesDeadlock;
t_dictionary* g_ESIsDeadlock;


/*------------------------------Consola------------------------------*/
void 				iniciarConsola							(void);
void 				ejecutarComando							(char *, bool *);

/*------------------------------Comandos------------------------------*/
void 				ejecutarMan								(void);
void 				pausarPlanificacion						(void);
void 				continuarPlanificacion					(void);
void 				bloquear								(char*);
void 				desbloquear								(char*);
void 				listarProcesos							(char*);
void 				killProceso								(char*);
void 				status									(char*);
void 				deadlock								(void);
void				salirConsola							(bool*);

/*------------------------------Auxiliares------------------------------*/
char* 				obtenerParametro						(char*, int);
char* 				obtenerId								(char*);

/*------------------------------Auxiliares-Estado de claves o ESI-----------------------------*/
bool 				estaListo								(char*);
bool 				estaBloqueadoPorLaClave					(char*, char*);
bool 				estaBloqueadaLaClave					(char*);
bool 				estaBloqueadoPorElESI					(char*, char*);
bool 				sonIgualesClaves						(char*);
bool				estaBloqueado							(char*);
bool 				enEjecucion								(char*);

/*------------------------------Auxiliares-desbloquear----------------------------*/
char* 				esiQueBloquea							(char*);
bool 				sonIgualesClaves						(char*);
void 				sacarClave								(char*, char*);

/*------------------------------Auxiliares-killProceso----------------------------*/
void        		siEstaBloqueadaPorClaveEliminar			(char*, t_list*);
void        		desbloqueoClave							(char*, t_list*);

/*------------------------------Auxiliares-Status------------------------------*/
void 				mostrarPorConsola						(t_respuestaStatus*);

/*------------------------------Auxiliares-Deadlock------------------------------*/
int 				indice									(char*, t_dictionary*);
int **				crearMatriz								(int, int);
void 				ponerMatrizTodoNulo						(int **, int, int);
void 				crearIndiceAsignacion					(char*);
void 				crearIndiceEspera						(t_infoBloqueo*);
void 				creoElementosEnPosibleDeadlock			(t_dictionary* diccionario,void (creoIndiceMatriz)(void));
void 				asignarEnMatrizAsignacion				(char*);
void 				asignarEnMatrizEspera					(t_infoBloqueo*);
void 				asignoMatriz							(t_dictionary* diccionario, void (asignarTipoMatriz)(void));
char* 				esiEnDeadlock							(int);

#endif /* CONSOLA_H_ */

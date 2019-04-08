#ifndef TABLAS_ADMINISTRATIVAS_H_
#define TABLAS_ADMINISTRATIVAS_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <commons/log.h>
#include <semaphore.h>
#include <pthread.h>

/*TABLA DE INSTANCIAS
 *
 *		{nombre de instancia, espacio ocupado(entradas), 	rango, 				 disponible, 	     ultimaModificada			trabajoActual   claves}
 *		 instancia1,          50 													97-106(a-j)    true     				18:12:1234							SET:k1045
 *		 instancia2			    	32						 							107-112(k-o)   true	  	 				18:12:4312							GET:k3042
 */

/*-------------------ESTRUCTURAS---------------------------*/
typedef int tiempo;
tiempo g_tiempoPorEjecucion;

typedef struct{
	char* nombre;
	int socket;
	pthread_t tid;
}t_conexion;

typedef struct {
	char *clave;
	char *valor;
} t_sentencia;

typedef struct{
	char* nombre;
	int espacioOcupado;
	int espacioMaximo;
	int tamanioEntrada;
	bool disponible;
	tiempo ultimaModificacion;
	int primerLetra;
	int ultimaLetra;
	t_sentencia *trabajoActual;
	t_list* claves;
	sem_t instanciaMutex;
}t_instancia;


sem_t g_mutex_tablas;

/*-------------------FUNCIONES---------------------------*/

t_instancia* traerInstanciaMasEspacioDisponible(t_list* tablaDeInstancias);
t_instancia* traerUltimaInstanciaUsada(t_list* tablaDeInstancias);
t_instancia* traerInstanciaQueContieneKey(t_list* tablaDeInstancia,char* primerLetraClave);
void distribuirKeys(t_list* tablaDeInstancias);

t_list* crearListaInstancias(void);
t_instancia* crearInstancia(char* nombre, int espacioMaximo, int tamanioEntradas);
void agregarInstancia(t_list * lista, t_instancia* instancia);
void destruirInstancia(t_instancia * instancia);
void mostrarInstancia(t_instancia * instancia);
void mostrarEspacioOcupado(t_instancia* instancia);
void mostrarTablaInstancia(t_list* tablaDeInstancias);
t_instancia* buscarInstancia(t_list* tablaDeInstancias,bool buscaInstanciasNoDisponibles ,char* nombre,int letraAEncontrar, char* clave);
void eliminiarClaveDeInstancia(t_instancia* instancia, char* claveAEliminar);
void agregarClaveDeInstancia(t_instancia* instancia, char* claveAEliminar);
void agregarTrabajoActual(t_instancia* instancia, char* clave, char* valor);
t_sentencia* conseguirTrabajoActual(t_instancia* instancia);
bool instanciaContieneClave(t_list* claves,char* clave);
int cantidadEntradasPorClave(char* clave, int tamanioEntradas);
void bloquearInstancia(t_instancia* instancia);
void desbloquearInstancia(t_instancia* instancia);
void bloquearTodasLasInstancias(t_list* tablaDeInstancias);
void desbloquearTodasLasInstancias(t_list* tablaDeInstancias);

void bloquearPeticion(sem_t* mutexPeticion);
void desbloquearPeticion(sem_t* mutexPeticion);

t_list* crearDiccionarioConexiones(void);
t_conexion* crearConexion(char* nombre, int socket);
void agregarConexion(t_list * diccionario, char * clave , int valor);
void sacarConexion(t_list* diccionario, t_conexion* conexion);
void mostrarDiccionario(t_list* diccionario);
t_conexion* buscarConexion(t_list * diccionario, char * clave, int socket);
void cerrarConexion(void* conexion);
void destruirConexion(void* conexion);
void cerrarTodasLasConexiones(t_list* diccionario);
void destruirDiccionario(t_list* diccionario);

t_list* crearDiccionarioClaves();
void agregarClaveAlSistema(t_list* diccionarioClaves, char* clave);
bool existeClaveEnSistema(t_list* diccionarioClaves, char* clave);
void destruirDiccionarioClaves(t_list* diccionarioClaves);

#endif /* TABLAS_ADMINISTRATIVAS_H_ */

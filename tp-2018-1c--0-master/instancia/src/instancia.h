#ifndef INSTANCIA_H_
#define INSTANCIA_H_

#include <biblioteca/sockets.h>
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>

/*------------------------Constantes-------------------------*/
#define RUTA_CONFIG "/home/utnso/workspace/tp-2018-1c--0/configuraciones/instancia.cfg"

/*------------------------Variables globales-------------------------*/
t_log * logInstancia;

int socketCoordinador;

char * algoritmoReemplazo;
char * puntoMontaje;
int intervaloDump;

bool recibirSolicitudes;

int cantEntradas;
int tamanioEntrada;

void * storage;
bool * bitMap;

t_list * tablaEntradas;

int entradaAReemplazar;

pthread_mutex_t mutexDumpCompactacion;

bool almacenar;

pthread_t threadAlmacenamientoContinuo;

/*------------------------Estructuras-------------------------*/
typedef struct {
	char clave[40];
	void * entrada;
	int tamanio;
	int indexComienzo;
	int tiempoReferenciado;
} t_tabla_entradas;

enum errores {
	ENTRADA_INEXISTENTE = -10,
	CANTIDAD_INDEX_LIBRES_INEXISTENTES,
};


/*-------------------------Conexion-------------------------*/
void 		conectarInstancia	(void);
t_config* 	leerConfiguracion	(void);

/*-------------------------Procesamiento paquetes-------------------------*/
void 		procesarPaquete				(t_paquete * unPaquete, int * client_socket);
void 		procesarEnviarInfoInstancia	(t_paquete * unPaquete);
void 		procesarSet					(t_paquete * unPaquete, int client_socket);
void 		procesarSetDefinitivo		(t_paquete * unPaquete, int client_socket);
void 		procesarStore				(t_paquete * unPaquete, int client_socket);
void 		procesarCompactacion		(t_paquete * unPaquete, int client_socket);
void 		procesarSolicitudValor		(t_paquete * unPaquete, int client_socket);
void 		procesarError				(t_paquete * unPaquete);

/*-------------------------Tabla de entradas-------------------------*/
void 				crearTablaEntradas						(void);
void 				destruirTablaEntradas					(void);
t_tabla_entradas * 	buscarEntrada							(char * clave);
void 				eliminarClave							(char * clave);
void 				mostrarTablaEntradas					(void);
int 				agregarClaveValor						(char * clave, void * valor);
void * 				buscarValorSegunClave					(char * clave);
t_tabla_entradas *	buscarEntradaSegunIndex					(int index);
void 				mostrarEntrada							(char * clave);
void 				aumentarTiempoReferenciado				(t_tabla_entradas * entrada);
void 				aumentarTiempoReferenciadoATodos		(t_list * tabla);
void				aumentarTiempoReferenciadoMenosAClave	(char * clave);

/*-------------------------BitMap del Storage-------------------------*/
void 				crearBitMap					(void);
void 				destruirBitMap				(void);
void 				liberarBitMap				(void);
void 				llenarBitMap				(void);
void 				liberarIndex				(int index);
void 				ocuparIndex					(int index);
int 				buscarIndexLibre			(void);
void 				mostrarBitmap				(void);
int 				buscarCantidadIndexLibres	(int cantidad);
int					cantidadIndexLibres			(void);

/*-------------------------Storage-------------------------*/
void				crearStorage				(void);
void 				destruirStorage				(void);
void * 				guardarEnStorage			(void * valor, int * index);
void * 				guardarEnStorageEnIndex		(void * valor, int index);
void 				compactar					(void);

/*-------------------------Dump-------------------------*/
void 				dump							(void);
void 				almacenamientoContinuo			(void);
void 				crearAlmacenamientoContinuo		(void);
void 				recuperarInformacionDeInstancia	(t_list * listaClaves);
void				almacenarEnMemoriaSecundaria	(t_tabla_entradas * registroEntrada);

/*-------------------------Algoritmos de reemplazo-------------------------*/
void 				algoritmoReemplazoCircular			(char * clave, void * valor);
void 				algoritmoReemplazoBiggestSpaceUsed	(char * clave, void * valor);
void 				algoritmoReemplazoLeastRecentlyUsed	(char * clave, void * valor);
t_list * 			ordenarEntradasAtomicasParaCircular	(void);
t_list * 			ordenarEntradasAtomicasParaBSU		(void);
t_list * 			ordenarEntradasAtomicasParaLRU		(void);
void 				eliminarEntradasParaReemplazo		(t_list * entradasAtomicas, void * valor);
t_list * 			desempate							(t_tabla_entradas * entrada, t_tabla_entradas * entrada2);

/*-------------------------Señales-------------------------*/
void				procesarSIGINT		(void);

/*-------------------------Funciones auxiliares-------------------------*/
void * 				abrirArchivo					(char * rutaArchivo, size_t * tamArc, FILE ** archivo);
t_list * 			listarArchivosDeMismaCarpeta	(char * ruta);
int 				entradasNecesariaParaUnTamanio	(int tamanio);
t_list * 			filtrarEntradasAtomicas			(void);

#endif /* INSTANCIA_H_ */

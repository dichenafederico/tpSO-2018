#include "consola.h"

/*------------------------------Consola------------------------------*/
void iniciarConsola() {
	char* linea;
	bool ejecutar = true;

	while (ejecutar) {
		linea = readline(">");

		if (linea) {
			add_history(linea);
		} else {
			free(linea);
			break;
		}

		ejecutarComando(linea, &ejecutar);

		free(linea);
	}

	clear_history();
}

void ejecutarComando(char* linea, bool* ejecutar) {

	char* Comando = obtenerParametro(linea, 0);

	// MAN
	if (strcmp(Comando, "man") == 0) {
		ejecutarMan();
		free(Comando);
		return;
	}

	// PAUSAR PLANIFICACIÓN
	if (strcmp(Comando, "pausar") == 0) {
		pausarPlanificacion();
		free(Comando);
		return;
	}

	// CONTINUAR PLANIFICACIÓN
	if (strcmp(Comando, "continuar") == 0) {
		continuarPlanificacion();
		free(Comando);
		return;
	}

	// BLOQUEAR
	if (strcmp(Comando, "bloquear") == 0) {
		bloquear(linea);
		free(Comando);
		return;
	}

	// DESBLOQUEAR
	if (strcmp(Comando, "desbloquear") == 0) {
		desbloquear(linea);
		free(Comando);
		return;
	}

	// LISTAR PROCESOS
	if (strcmp(Comando, "listar") == 0) {
		listarProcesos(linea);
		free(Comando);
		return;
	}

	// KILL PROCESO
	if (strcmp(Comando, "kill") == 0) {
		killProceso(linea);
		free(Comando);
		return;
	}

	// STATUS
	if (strcmp(Comando, "status") == 0) {
		status(linea);
		free(Comando);
		return;
	}

	// DEADLOCK
	if (strcmp(Comando, "deadlock") == 0) {
		deadlock();
		free(Comando);
		return;
	}

	// SALIR DE LA CONSOLA
	if (strcmp(Comando, "exit") == 0) {
		salirConsola(ejecutar);
		free(Comando);
		return;
	}

	// NO RECONOCER COMANDO
	printf("No se ha encontrado el comando %s \n", Comando);
	free(Comando);
}

/*-------------------------------Comandos------------------------------*/
void ejecutarMan() {
	printf("NAME \n");
	printf("	consola \n\n");

	printf("SYNOPSIS \n");
	printf("	#include <consola.h> \n\n");

	printf("	void pausar(void) \n");
	printf("	void continuar(void) \n");
	printf("	void bloquear(char* clave, char* nombreESI) \n");
	printf("	void desbloquear(char* clave, char* nombreESI) \n");
	printf("	void listar(char* clave) \n");
	printf("	void kill(char* nombreESI) \n");
	printf("	void status(char* clave) \n");
	printf("	void deadlock(void) \n");
	printf("	void exit(void) \n\n");

	printf("DESCRIPTION \n");
	printf("	pausar --> Pausar planificacion \n");
	printf("	continuar --> Continuar planificacion \n");
	printf(
			"	bloquear --> Se bloqueara el proceso ESI hasta ser desbloqueado \n");
	printf(
			"	desbloquear -->  Se desbloqueara el proceso ESI con el ID especificado \n");
	printf("	listar --> Lista los procesos encolados esperando al recurso \n");
	printf("	kill --> Finaliza el proceso ESI \n");
	printf(
			"	status -->  Brinda informacion sobre las instancias del sistema \n");
	printf("	deadlock -->   Muestra los procesos ESI en deadlock\n");
	printf("	exit --> Cierra la consola \n\n");
}
void salirConsola(bool* ejecutar) {
	printf("Se cerro la consola \n");
	*ejecutar = false;
}

void pausarPlanificacion(void) {
	printf("Planificador pausado.\n");
	pthread_mutex_lock(&mutexConsola);

}

void continuarPlanificacion(void) {
	printf("Planificador corriendo.\n");
	pthread_mutex_unlock(&mutexConsola);
}

void bloquear(char* linea) {
	char* clave = obtenerParametro(linea, 1);

	if (clave == NULL)
		return;

	char* nombreESI = obtenerParametro(linea, 2);

	if (nombreESI == NULL) {
		free(clave);
		return;
	}

	if (estaBloqueadoPorLaClave(nombreESI, clave)) {
		printf("Ya esta bloqueado el %s por la clave %s.\n", nombreESI, clave);
		free(clave);
		free(nombreESI);
		return;
	}

	if (estaListo(nombreESI) || enEjecucion(nombreESI)) {

		pthread_mutex_lock(&mutexClavesTomadas);
		if (estaBloqueadoPorElESI(clave, nombreESI)) {
			pthread_mutex_unlock(&mutexClavesTomadas);
			printf("El %s ya tiene la clave %s tomada.\n", nombreESI, clave);
			free(clave);
			free(nombreESI);
			return;
		}
		pthread_mutex_unlock(&mutexClavesTomadas);
	}

	else {
		printf("Solo se puede bloquear el %s en estado listo o en ejecucion.\n",
				nombreESI);
		free(clave);
		free(nombreESI);
		return;
	}

	if (enEjecucion(nombreESI)) {
		pthread_mutex_lock(&mutexInstruccionConsola);
		g_claveGET = strdup(clave);
		g_bloqueo = 1;
		g_instruccionConsola = 1;
		sem_post(&continua);
		pthread_mutex_unlock(&mutexInstruccionConsola);
		free(clave);
		free(nombreESI);
		return;
	}

	// Si la clave no estaba bloqueada, se bloquea para el idESI
	if (!estaBloqueadaLaClave(clave)) {
		// Se agrega al diccionario de clavesBloqueadas por el idESI la clave a bloquear
		if (!dictionary_has_key(g_clavesTomadas, nombreESI)) {
			t_list* listaVacia = list_create();

			pthread_mutex_lock(&mutexClavesTomadas);
			dictionary_put(g_clavesTomadas, nombreESI, listaVacia);
			pthread_mutex_unlock(&mutexClavesTomadas);

		}

		pthread_mutex_lock(&mutexClavesTomadas);
		list_add(dictionary_get(g_clavesTomadas, nombreESI), strdup(clave));
		pthread_mutex_unlock(&mutexClavesTomadas);

	}

	// Si la clave esta bloqueada, se coloca al idESI en la lista de esis esperando a esa clave
	else {
		if (!dictionary_has_key(g_bloq, clave)) {
			t_list* listaVacia = list_create();

			pthread_mutex_lock(&mutexBloqueo);
			dictionary_put(g_bloq, clave, listaVacia);
			pthread_mutex_unlock(&mutexBloqueo);

		}

		// Se pasa el ESI del estado listo a Bloqueados
		t_infoBloqueo* insertar = malloc(sizeof(t_infoBloqueo));
		insertar->idESI = strdup(obtenerId(nombreESI));

		pthread_mutex_lock(&mutexListo);
		insertar->data = dictionary_remove(g_listos, insertar->idESI);

		pthread_mutex_unlock(&mutexListo);
		pthread_mutex_lock(&mutexBloqueo);
		list_add(dictionary_get(g_bloq, clave), insertar);
		pthread_mutex_unlock(&mutexBloqueo);

	}

	free(nombreESI);
	free(clave);
}

void desbloquear(char* linea) {

	char* clave = obtenerParametro(linea, 1);

	if (clave == NULL)
		return;

	pthread_mutex_lock(&mutexBloqueo);
	if (!dictionary_has_key(g_bloq, clave)
			|| list_is_empty(dictionary_get(g_bloq, clave))) {
		pthread_mutex_unlock(&mutexBloqueo);
		printf(
				"No se puede desbloquear un ESI de la clave %s que no esta bloqueada.\n",
				clave);
		free(clave);
		return;
	}
	pthread_mutex_unlock(&mutexBloqueo);

	desbloquearESI(clave);

	free(clave);
}

void listarProcesos(char* linea) {
	char* clave = obtenerParametro(linea, 1);

	if (clave == NULL)
		return;

	pthread_mutex_lock(&mutexBloqueo);
	if (!dictionary_has_key(g_bloq, clave)
			|| list_is_empty(dictionary_get(g_bloq, clave))) {
		pthread_mutex_unlock(&mutexBloqueo);
		printf("Ningun ESI esta bloqueado por la clave %s.\n", clave);
		free(clave);
		return;
	}
	pthread_mutex_unlock(&mutexBloqueo);

	printf("Claves de las ESIs bloqueadas por la clave %s.\n", clave);

	int i;

	pthread_mutex_lock(&mutexBloqueo);
	for (i = 0; i < list_size(dictionary_get(g_bloq, clave)); i++) {
		printf("	%s \n",
				((t_infoBloqueo*) list_get(dictionary_get(g_bloq, clave), i))->data->nombreESI);
	}
	pthread_mutex_unlock(&mutexBloqueo);

	// Libero memoria
	free(clave);
}

void killProceso(char* linea) {
	char* nombreESI = obtenerParametro(linea, 1);

	if (nombreESI == NULL)
		return;

	if (enEjecucion(nombreESI)) {
		pthread_mutex_lock(&mutexInstruccionConsola);
		g_termino = 1;
		sem_post(&continua);
		pthread_mutex_unlock(&mutexInstruccionConsola);
		free(nombreESI);
	}

	int obtenerSocket(char* nombreESI) {

		if (estaListo(nombreESI)) {
			return ((t_infoListos*) dictionary_get(g_listos,
					obtenerId(nombreESI)))->socketESI;
		} else {
			int socket;
			void buscarESI(char* clave, t_list* esisBLoqueados) {
				int j;
				for (j = 0; j < list_size(esisBLoqueados); j++) {
					t_infoBloqueo* esi = list_get(esisBLoqueados, j);
					if (strcmp(esi->data->nombreESI, nombreESI) == 0) {
						socket = esi->data->socketESI;
						break;
					}
					free(esi);
				}

			}
			dictionary_iterator(g_bloq, (void*) buscarESI);
			return socket;
		}
	}

	if (estaBloqueado(nombreESI) || estaListo(nombreESI)) {
		pthread_mutex_lock(&mutexListo);
		enviarRespuesta(obtenerSocket(nombreESI), ABORTO_ESI);
		pthread_mutex_unlock(&mutexListo);
	}

// Libero memoria
	free(nombreESI);
}

void status(char* linea) {

	char* clave = obtenerParametro(linea, 1);

	if (clave == NULL)
		return;

	enviarSolicitudStatus(g_socketCoordinador, clave);

// muestro ESIs bloqueados a la espera de dicha clave
	char* lineaExtra = string_new();
	string_append(&lineaExtra, "listar ");
	string_append(&lineaExtra, clave);
	listarProcesos(lineaExtra);

// Libero memoria
	free(clave);
	free(lineaExtra);
}

void deadlock() {
	g_clavesDeadlock = dictionary_create();
	g_ESIsDeadlock = dictionary_create();

// Se calcula el numero de filas y columnas
	creoElementosEnPosibleDeadlock(g_bloq, (void*) crearIndiceEspera);
	creoElementosEnPosibleDeadlock(g_clavesTomadas,
			(void*) crearIndiceAsignacion);

	int columEsperaFilasAsig, filasEsperaColumAsig;

	filasEsperaColumAsig = dictionary_size(g_ESIsDeadlock);
	columEsperaFilasAsig = dictionary_size(g_clavesDeadlock);

// Se crea espacio ambas matrices y las pongo en 0
	g_matrizEspera = crearMatriz(filasEsperaColumAsig, columEsperaFilasAsig);
	g_matrizAsignacion = crearMatriz(columEsperaFilasAsig,
			filasEsperaColumAsig);

	ponerMatrizTodoNulo(g_matrizEspera, filasEsperaColumAsig,
			columEsperaFilasAsig);
	ponerMatrizTodoNulo(g_matrizAsignacion, columEsperaFilasAsig,
			filasEsperaColumAsig);

// matrizEspera W: P -> R, los procesos P estan a la espera de recursos R
// Se pone en 1 los esis esperando en la matrizEspera
	asignoMatriz(g_bloq, (void*) asignarEnMatrizEspera);

// matrizAsignacion A: R -> P, los recursos R estan a la espera de procesos P
// Se pone en 1 las claves asignadas en la matrizAsignacion
	asignoMatriz(g_clavesTomadas, (void*) asignarEnMatrizAsignacion);

// Matriz de procesosAlaEsperaDeProcesos(T) es la composicion entre matrizEspera y matrizAsignacion (T=PxA T: P -> P)
	int i, j, k;

	int procesosAlaEsperaDeProcesos[filasEsperaColumAsig][filasEsperaColumAsig];
	for (i = 0; i < filasEsperaColumAsig; i++) {
		for (j = 0; j < filasEsperaColumAsig; j++) {
			procesosAlaEsperaDeProcesos[i][j] = 0;
			for (k = 0; k < columEsperaFilasAsig; k++) {
				if (g_matrizEspera[i][k] && g_matrizAsignacion[k][j]) {
					procesosAlaEsperaDeProcesos[i][j] = 1;
					k = columEsperaFilasAsig;
				}
			}
		}
	}

// Se calcula la matriz procesosAlaEsperaDeProcesos con cierre transitivo.
// Se calcula aplicandole el algoritmo Warshall a la matriz procesosAlaEsperaDeProcesos
	bool hayAlgunProcesoEnDeadlock = false;
	for (k = 0; k < filasEsperaColumAsig; k++) {
		for (i = 0; i < filasEsperaColumAsig; i++) {
			for (j = 0; j < filasEsperaColumAsig; j++) {
				procesosAlaEsperaDeProcesos[i][j] =
						procesosAlaEsperaDeProcesos[i][j]
								|| (procesosAlaEsperaDeProcesos[i][k]
										&& procesosAlaEsperaDeProcesos[k][j]);
				if (i == j && procesosAlaEsperaDeProcesos[i][j])
					hayAlgunProcesoEnDeadlock = true;
			}
		}
	}

	if (hayAlgunProcesoEnDeadlock) {
		puts("\nProcesos que estan en deadlock.\n");
		int cantDeadlock = 0;
		for (i = 0; i < filasEsperaColumAsig; i++) {
			if (procesosAlaEsperaDeProcesos[i][i]) {
				printf("Deadlock: %d\n", cantDeadlock);
				cantDeadlock++;
				for (j = 0; j < filasEsperaColumAsig; j++) {
					if (procesosAlaEsperaDeProcesos[i][j]) {
						if (procesosAlaEsperaDeProcesos[j][j]) {
							printf("%s\n", esiEnDeadlock(j));
						}
						procesosAlaEsperaDeProcesos[j][j] = 0;
					}
				}
				printf("\n");
			}
		}
	} else {
		puts("No hay ningun proceso en deadlock");
	}

// Libero memoria
	free((*g_matrizEspera));
	free((*g_matrizAsignacion));
	dictionary_destroy_and_destroy_elements(g_clavesDeadlock, (void*) free);
	dictionary_destroy_and_destroy_elements(g_ESIsDeadlock, (void*) free);

}

/*------------------------------Auxiliares------------------------------*/

char* obtenerParametro(char* linea, int parametro) {
	if (strcmp(linea, "") == 0)
		return "";

	char** palabras = string_split(linea, " ");

	if (palabras[parametro] == NULL) {
		int i;
		for (i = 0; palabras[i] != NULL; i++)
			free(palabras[i]);
		free(palabras);
		return NULL;
	}

	char* palabra = strdup(palabras[parametro]);

	int i;
	for (i = 0; palabras[i] != NULL; i++)
		free(palabras[i]);

	free(palabras);
	return palabra;

}

static void decirIdESI(t_infoBloqueo* infoESI) {
	if (strcmp(infoESI->data->nombreESI, g_nombreESI) == 0)
		g_idESI = infoESI->idESI;
}

static void averiguarIdESIBloq(char* clave, t_list* esisBloqueados) {
	list_iterate(esisBloqueados, (void*) decirIdESI);
}

static void averiguarIdESIListos(char* idESI, t_infoListos* infoESI) {
	if (strcmp(infoESI->nombreESI, g_nombreESI) == 0)
		g_idESI = idESI;
}

static char* obtenerIdESIEstado(char* nombreESI, t_dictionary* diccionario,
		void (averiguarIdESIEstado)(char, void*)) {
	g_nombreESI = nombreESI;

	dictionary_iterator(diccionario, (void*) averiguarIdESIEstado);
	return g_idESI;
}

char* obtenerId(char* nombreESI) {

	if (enEjecucion(nombreESI))
		return g_idESIactual;
	if (estaListo(nombreESI))
		return obtenerIdESIEstado(nombreESI, g_listos,
				(void*) averiguarIdESIListos);
	else
		return obtenerIdESIEstado(nombreESI, g_bloq, (void*) averiguarIdESIBloq);
}

/*------------------------------Auxiliares-Estado de claves o ESI-----------------------------*/

bool estaListo(char* nombreESI) {

	bool boolean = false;

	void _estaListoESI(char* idESI, t_infoListos* infoESIListo) {
		if (strcmp(nombreESI, infoESIListo->nombreESI) == 0) {
			boolean = true;
		}
	}

	dictionary_iterator(g_listos, (void*) _estaListoESI);

	return boolean;

}

bool estaBloqueadoPorLaClave(char* nombreESI, char* clave) {

	if (!dictionary_is_empty(g_bloq)) {
		pthread_mutex_lock(&mutexBloqueo);
		if (dictionary_has_key(g_bloq, clave)
				&& !list_is_empty(dictionary_get(g_bloq, clave))) {
			pthread_mutex_unlock(&mutexBloqueo);

			bool _sonIguales(t_infoBloqueo* infoBloqueo) {
				return (strcmp(infoBloqueo->data->nombreESI, nombreESI) == 0);
			}

			return list_any_satisfy(dictionary_get(g_bloq, clave),
					(void*) _sonIguales);
		} else {
			pthread_mutex_unlock(&mutexBloqueo);
			return false;
		}
	} else {
		return false;
	}
}

bool estaBloqueadoPorElESI(char* claveBloq, char* nombreESIBloq) {
	if (!dictionary_is_empty(g_clavesTomadas)) {
		if (dictionary_has_key(g_clavesTomadas, nombreESIBloq)) {

			free(g_clave);
			g_clave = claveBloq;

			return list_any_satisfy(
					dictionary_get(g_clavesTomadas, nombreESIBloq),
					(void*) sonIgualesClaves);
		} else {
			return false;
		}
	} else {
		return false;
	}
}

bool estaBloqueadaLaClave(char* clave) {
	bool boolean = false;

	if (dictionary_has_key(g_bloq, clave))
		return true;

	void _estaClaveBloqueada(char* nombreESI, t_list* clavesBloqueadas) {
		if (!boolean) {
			boolean = estaBloqueadoPorElESI(clave, nombreESI);
		}
	}

	dictionary_iterator(g_clavesTomadas, (void*) _estaClaveBloqueada);

	return boolean;
}

bool estaBloqueado(char* nombreESI) {
	bool boolean = false;

	void _estaESIBloquedo(char* clave, t_list* esisBloqueados) {
		if (boolean == false) {
			boolean = estaBloqueadoPorLaClave(nombreESI, clave);
		}
	}

	dictionary_iterator(g_bloq, (void*) _estaESIBloquedo);

	return boolean;
}

bool enEjecucion(char* nombreESI) {
	if (estaBloqueado(nombreESI) || g_nombreESIactual == NULL)
		return false;
	return (strcmp(nombreESI, g_nombreESIactual) == 0);
}

/*------------------------------Auxiliares-desbloquear----------------------------*/

bool sonIgualesClaves(char* claveActual) {
	return (strcmp(g_clave, claveActual) == 0);
}

/*------------------------------Auxiliares-Status------------------------------*/
void mostrarPorConsola(t_respuestaStatus* respuestaStatus) {
	if (strcmp(respuestaStatus->valor, "") == 0)
		puts("No posee valor.");
	else
		printf("Valor: %s\n", respuestaStatus->valor);

	if (strcmp(respuestaStatus->nomInstanciaActual, "") == 0)
		puts("No se encuentra en ninguna instancia.");
	else
		printf("Instancia actual en donde esta clave: %s\n",
				respuestaStatus->nomInstanciaActual);

		printf("Instancia en donde se guardaria clave: %s\n",
				respuestaStatus->nomIntanciaPosible);

}

/*------------------------------Auxiliares-Deadlock------------------------------*/

int indice(char* elemento, t_dictionary* diccionario) {
	int indice;
	if (!dictionary_has_key(diccionario, elemento)) {
		// creo una clave nueva
		indice = dictionary_size(diccionario);
		dictionary_put(diccionario, elemento, string_itoa(indice));
	} else {
		indice = atoi(dictionary_get(diccionario, elemento));
	}
	return indice;
}

int **crearMatriz(int nroFilas, int nroColum) {
	int **matriz = (int **) calloc(nroFilas, sizeof(int*));
	int i;
	for (i = 0; i < nroFilas; i++)
		matriz[i] = (int *) calloc(nroColum, sizeof(int));
	return matriz;
}

void ponerMatrizTodoNulo(int ** matriz, int nroFilas, int nroColum) {
	int i, j;
	for (i = 0; i < nroFilas; i++) {
		for (j = 0; j < nroColum; j++) {
			matriz[i][j] = 0;
		}
	}
}

void crearIndiceAsignacion(char* clave) {
	indice(clave, g_clavesDeadlock);
}

void crearIndiceEspera(t_infoBloqueo* esiBloqueado) {
	indice(esiBloqueado->data->nombreESI, g_ESIsDeadlock);
}

void creoElementosEnPosibleDeadlock(t_dictionary* diccionario,
		void (creoIndiceMatriz)(void)) {
	void _creoElementoEnPosibleDeadlock(char* elemento, t_list* lista) {
		if (!list_is_empty(lista)) {
			list_iterate(lista, (void*) creoIndiceMatriz);
			indice(elemento, diccionario);
		}
	}

	dictionary_iterator(diccionario, (void*) _creoElementoEnPosibleDeadlock);
}

void asignarEnMatrizAsignacion(char* clave) {
	g_matrizAsignacion[indice(clave, g_clavesDeadlock)][indice(g_elemento,
			g_ESIsDeadlock)] = 1;
}

void asignarEnMatrizEspera(t_infoBloqueo* infoESIBloqueado) {
	g_matrizEspera[indice(infoESIBloqueado->data->nombreESI, g_ESIsDeadlock)][indice(
			g_elemento, g_clavesDeadlock)] = 1;
}

void asignoMatriz(t_dictionary* diccionario, void (asignarTipoMatriz)(void)) {
	void _asignarMatriz(char* elemento, t_list* lista) {
		if (!list_is_empty(lista)) {
			free(g_elemento);
			g_elemento = strdup(elemento);
			list_iterate(lista, (void*) asignarTipoMatriz);
		}
	}

	dictionary_iterator(diccionario, (void*) _asignarMatriz);
}

char* esiEnDeadlock(int posicionMatriz) {
	char* nombreESIEnDeadlock;

	void _buscarNombreESIEnDeadlock(char* nombreESI, char* indiceDeadlock) {
		if (posicionMatriz == atoi(indiceDeadlock))
			nombreESIEnDeadlock = nombreESI;
	}

	dictionary_iterator(g_ESIsDeadlock, (void*) _buscarNombreESIEnDeadlock);

	return nombreESIEnDeadlock;
}
